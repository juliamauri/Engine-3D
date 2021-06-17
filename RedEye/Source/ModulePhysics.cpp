#include "ModulePhysics.h"

#include "RE_Profiler.h"
#include "Application.h"
#include "ModuleScene.h"
#include "RE_PrimitiveManager.h"
#include "RE_Time.h"
#include "RE_CompCamera.h"

ModulePhysics::ModulePhysics() : Module("Physics") {}
ModulePhysics::~ModulePhysics() {}

void ModulePhysics::Update()
{
	RE_PROFILE(PROF_Update, PROF_ModulePhysics);
	const float global_dt = RE_TIME->GetDeltaTime();

	switch (mode) {
	case ModulePhysics::FIXED_UPDATE:
	{
		dt_offset += global_dt;

		float final_dt = 0.f;
		while (dt_offset >= fixed_dt)
		{
			dt_offset -= fixed_dt;
			final_dt += fixed_dt;
		}

		if (final_dt > 0.f)
		{
			update_count++;
			particles.Update(final_dt);
		}

		break;
	}
	case ModulePhysics::FIXED_TIME_STEP:
	{
		dt_offset += global_dt;

		while (dt_offset >= fixed_dt)
		{
			dt_offset -= fixed_dt;
			update_count++;
			particles.Update(fixed_dt);
		}

		break;
	}
	default:
	{
		update_count++;
		particles.Update(global_dt);
		break;
	}
	}

	time_counter += global_dt;
	if (time_counter >= 1.f)
	{
		time_counter--;
		updates_per_s = update_count;
		update_count = 0.f;
	}
}

void ModulePhysics::CleanUp()
{
	particles.Clear();
}

void ModulePhysics::DrawDebug(RE_CompCamera* current_camera) const
{
	if (!particles.simulations.empty())
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(current_camera->GetProjectionPtr());
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((current_camera->GetView()).ptr());
		particles.DrawDebug();
	}
}

void ModulePhysics::DrawEditor()
{
	if (ImGui::CollapsingHeader(name))
	{
		ImGui::Text("Updates/s: %.1f", updates_per_s);

		int type = static_cast<int>(mode);
		if (ImGui::Combo("Update Mode", &type, "Engine Par\0Fixed Update\0Fixed Time Step\0"))
			mode = static_cast<UpdateMode>(type);

		if (mode)
		{
			float period = 1.f / fixed_dt;
			if (ImGui::DragFloat("Dt", &period, 1.f, 1.f, 480.f, "%.0f"))
				fixed_dt = 1.f / period;
		}

		particles.DrawEditor();
	}
}

RE_ParticleEmitter* ModulePhysics::AddEmitter()
{
	RE_ParticleEmitter* ret = new RE_ParticleEmitter();

	// Prim setup
	ret->primCmp = new RE_CompPoint();
	RE_SCENE->primitives->SetUpComponentPrimitive(ret->primCmp);

	particles.Allocate(ret);
	return ret;
}

void ModulePhysics::RemoveEmitter(RE_ParticleEmitter* emitter)
{
	particles.Deallocate(emitter->id);
}

unsigned int ModulePhysics::GetParticleCount(unsigned int emitter_id) const
{
	for (auto sim : particles.simulations)
		if (sim->id == emitter_id)
			return sim->particle_count;

	return 0u;
}

eastl::list<RE_Particle*>* ModulePhysics::GetParticles(unsigned int emitter_id) const
{
	for (auto sim : particles.simulations)
		if (sim->id == emitter_id)
			return &sim->particle_pool;

	return nullptr;
}
