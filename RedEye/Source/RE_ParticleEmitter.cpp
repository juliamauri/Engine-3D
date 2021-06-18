#include "RE_ParticleEmitter.h"

#include "Application.h"
#include "RE_Math.h"

RE_ParticleEmitter::BoundingMode RE_ParticleEmitter::mode = PER_PARTICLE;

unsigned int RE_ParticleEmitter::Update(const float global_dt)
{
	RE_PROFILE(PROF_Update, PROF_ParticleEmitter);
	switch (state)
	{
	case RE_ParticleEmitter::STOPING:
	{
		Reset();
		state = RE_ParticleEmitter::STOP;
		break;
	}
	case RE_ParticleEmitter::RESTART:
	{
		Reset();
		state = RE_ParticleEmitter::PLAY;
		break;
	}
	case RE_ParticleEmitter::PLAY:
	{
		if (IsTimeValid(global_dt))
		{
			UpdateParticles();
			UpdateSpawn();
		}
		break;
	}
	default: break; }

	return particle_count;
}

void RE_ParticleEmitter::Reset()
{
	if (!particle_pool.empty()) particle_pool.clear();

	particle_count = 0u;
	max_dist_sq = max_speed_sq = total_time = 
	spawn_interval.time_offset = spawn_mode.time_offset = 0.f;
	spawn_mode.has_started = false;
	bounding_box.minPoint = bounding_box.maxPoint = math::vec::zero;
}

bool RE_ParticleEmitter::IsTimeValid(const float global_dt)
{
	RE_PROFILE(PROF_ParticleTiming, PROF_ParticleEmitter);
	// Check time limitations
	local_dt = global_dt * time_muliplier;
	if (total_time < start_delay)
	{
		if (total_time + local_dt >= start_delay)
		{
			total_time += local_dt;
			local_dt -= start_delay - total_time;
		}
		else return false;
	}
	else total_time += local_dt;

	if (!loop && total_time >= max_time)
	{
		state = RE_ParticleEmitter::STOPING;
		return false;
	}

	return local_dt > 0.f;
}

void RE_ParticleEmitter::UpdateParticles()
{
	RE_PROFILE(PROF_ParticleUpdate, PROF_ParticleEmitter);

	max_dist_sq = max_speed_sq = 0.f;
	bounding_box.minPoint = bounding_box.maxPoint = parent_pos * !local_space;

	for (eastl::list<RE_Particle*>::iterator p1 = particle_pool.begin(); p1 != particle_pool.end();)
	{
		// Check if particle is still alive
		bool is_alive = true;
		switch (initial_lifetime.type) {
		case RE_EmissionSingleValue::Type::VALUE: is_alive = ((*p1)->lifetime += local_dt) < initial_lifetime.GetValue(); break;
		case RE_EmissionSingleValue::Type::RANGE: is_alive = ((*p1)->lifetime += local_dt) < (*p1)->max_lifetime; break;
		default: break; }

		// Iterate collisions
		if (is_alive)
		{
			switch (collider.type) {
			case RE_EmissionCollider::Type::POINT:
			{
				if (collider.inter_collisions)
					for (eastl::list<RE_Particle*>::iterator p2 = p1.next(); p2 != particle_pool.end(); ++p2)
						ImpulseCollision(**p1, **p2);

				is_alive = boundary.PointCollision(**p1);
				break;
			}
			case RE_EmissionCollider::Type::SPHERE:
			{
				if (collider.inter_collisions)
					for (eastl::list<RE_Particle*>::iterator p2 = p1.next(); p2 != particle_pool.end(); ++p2)
						ImpulseCollision(**p1, **p2, (*p1)->col_radius + (*p2)->col_radius);

				is_alive = boundary.SphereCollision(**p1);
				break;
			}
			default:
			{
				is_alive = boundary.PointCollision(**p1);
				break;
			}
			}
		}

		if (is_alive)
		{
			// Update Speed & Position
			(*p1)->velocity += external_acc.GetAcceleration() * local_dt;
			(*p1)->position += (*p1)->velocity * local_dt;

			// Update Control values
			max_dist_sq = RE_Math::MaxF(max_dist_sq, ((*p1)->position - (parent_pos * !local_space)).LengthSq());
			max_speed_sq = RE_Math::MaxF(max_speed_sq, (*p1)->velocity.LengthSq());

			// Broadphase AABB Boundary
			if (RE_ParticleEmitter::mode == PER_PARTICLE)
			{
				bounding_box.maxPoint = RE_Math::MaxVecValues((*p1)->position, bounding_box.maxPoint);
				bounding_box.minPoint = RE_Math::MinVecValues((*p1)->position, bounding_box.minPoint);
			}

			++p1;
		}
		else // Remove dead particles
		{
			DEL(*p1);
			p1 = particle_pool.erase(p1);
			particle_count--;
		}
	}

	if (RE_ParticleEmitter::mode == GENERAL)
	{
		switch (boundary.type) {
		case RE_EmissionBoundary::Type::SPHERE: bounding_box.SetFrom(boundary.geo.sphere); break;
		case RE_EmissionBoundary::Type::AABB: bounding_box = boundary.geo.box; break;
		default: bounding_box.SetFromCenterAndSize(parent_pos * !local_space, math::vec(math::SqrtFast(max_dist_sq))); break; }
	}
}

void RE_ParticleEmitter::UpdateSpawn()
{
	RE_PROFILE(PROF_ParticleSpawn, PROF_ParticleEmitter);
	if (spawn_interval.IsActive(local_dt))
	{
		const unsigned int to_add = RE_Math::MinUI(
			spawn_mode.CountNewParticles(local_dt),
			max_particles - particle_count);

		particle_count += to_add;
		for (unsigned int i = 0u; i < to_add; ++i)
			particle_pool.push_back(new RE_Particle(
				initial_lifetime.GetValue(),
				local_space ? initial_pos.GetPosition() : initial_pos.GetPosition() + parent_pos,
				!inherit_speed ? initial_speed.GetValue() : initial_speed.GetValue() + parent_speed,
				collider.mass.GetValue(), collider.radius.GetValue(), collider.restitution.GetValue(),
				light.GetColor(), light.GetIntensity(), light.GetSpecular()));
	}
}

void RE_ParticleEmitter::ImpulseCollision(RE_Particle& p1, RE_Particle& p2, const float combined_radius) const
{
	RE_PROFILE(PROF_ParticleCollision, PROF_ParticleEmitter);
	// Check particle collision
	const math::vec collision_dir = p1.position - p2.position;
	const float dist2 = collision_dir.Dot(collision_dir);
	if (dist2 <= combined_radius * combined_radius)
	{

#ifdef PARTICLE_PHYSICS_TEST
		ProfilingTimer::p_col_internal++;
#endif // PARTICLE_PHYSICS_TEST

		// Get mtd: Minimum Translation Distance
		const float dist = math::Sqrt(dist2);
		const math::vec mtd = collision_dir * (combined_radius - dist) / dist;

		// Resolve Intersection
		const float p1_inv_mass = 1.f / p1.mass;
		const float p2_inv_mass = 1.f / p2.mass;
		p1.position += mtd * (p1_inv_mass / (p1_inv_mass + p2_inv_mass));
		p2.position -= mtd * (p2_inv_mass / (p1_inv_mass + p2_inv_mass));

		// Resolve Collision
		const math::vec col_normal = collision_dir.Normalized();
		const math::vec col_speed = (p1.velocity - p2.velocity);
		const float dot = col_speed.Dot(col_normal);

		// Check if particles not already moving away
		if (dot < 0.0f)
		{
			// Resolve applying impulse
			const math::vec impulse = col_normal * (-(p1.col_restitution + p2.col_restitution) * dot) / (p1_inv_mass + p2_inv_mass);
			p1.velocity += impulse * p1_inv_mass;
			p2.velocity -= impulse * p2_inv_mass;
		}
	}
}

#if defined(PARTICLE_PHYSICS_TEST) || defined(PARTICLE_RENDER_TEST)

RE_ParticleEmitter* RE_ParticleEmitter::demo_emitter = nullptr;

void RE_ParticleEmitter::DemoSetup()
{
	Reset();

	int i = ++ProfilingTimer::current_sim;
	ProfilingTimer::p_count = 0u;

	state = PlaybackState::STOP;
	initial_lifetime.type = RE_EmissionSingleValue::Type::NONE;

#ifdef PARTICLE_PHYSICS_TEST

	active_rendering = false;
	ProfilingTimer::p_col_internal = ProfilingTimer::p_col_boundary = 0u;
	initial_pos.type = RE_EmissionShape::Type::CIRCLE;
	collider.type = static_cast<RE_EmissionCollider::Type>(1 + (i % 6 < 3));
	collider.inter_collisions = (i < 6);

#ifdef DEBUG

	spawn_mode.frequency = i < 6 ? 10.f : 980.f;
	initial_pos.geo.circle = math::Circle(math::vec::zero, { 0.f, 1.f, 0.f }, i < 6 ? 12.f : 60.f);

	switch (boundary.type = static_cast<RE_EmissionBoundary::Type>(1 + (i % 3))) {
	case RE_EmissionBoundary::PLANE: boundary.geo.plane = math::Plane({ 0.f, 1.f, 0.f }, i < 6 ? -10.f : -40.f); break;
	case RE_EmissionBoundary::SPHERE: boundary.geo.sphere = math::Sphere(math::vec::zero, i < 6 ? 20.f : 70.f); break;
	case RE_EmissionBoundary::AABB: boundary.geo.box.SetFromCenterAndSize(math::vec::zero, math::vec(i < 6 ? 30.f : 140.f)); break;
	default: break; }

#else // RELEASE

	spawn_mode.frequency = i < 6 ? 120.f : 2000.f;
	initial_pos.geo.circle = math::Circle(math::vec::zero, { 0.f, 1.f, 0.f }, i < 6 ? 40.f : 260.f);

	switch (boundary.type = static_cast<RE_EmissionBoundary::Type>(1 + (i % 3))) {
	case RE_EmissionBoundary::PLANE: boundary.geo.plane = math::Plane({ 0.f, 1.f, 0.f }, i < 6 ? -20.f : -70.f); break;
	case RE_EmissionBoundary::SPHERE: boundary.geo.sphere = math::Sphere(math::vec::zero, i < 6 ? 80.f : 400.f); break;
	case RE_EmissionBoundary::AABB: boundary.geo.box.SetFromCenterAndSize(math::vec::zero, math::vec(i < 6 ? 120.f : 550.f)); break;
	default: break; }

#endif

#else // PARTICLE_RENDER_TEST

	// TODO JULIUS: setup emitter properties for rendering profiling
	



#endif
}

#endif // PARTICLE_PHYSICS_TEST || PARTICLE_RENDER_TEST
