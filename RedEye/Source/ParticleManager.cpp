#include "ParticleManager.h"

#include "Application.h"
#include "RE_Math.h"

unsigned int ParticleManager::emitter_count = 0u;

bool RE_Particle::Update(float dt)
{
	bool is_alive = ((lifetime += dt) < max_lifetime);

	if (is_alive)
	{
		position += speed * dt;
	}

	return is_alive;
}

int RE_ParticleEmitter::GetNewSpawns(float dt)
{
	int units = static_cast<int>((spawn_offset += dt) * spaw_frequency);
	spawn_offset -= static_cast<float>(units) / spaw_frequency;
	return units;
}

void RE_ParticleEmitter::SetUpParticle(RE_Particle* particle)
{
	if(randomLightColor)
		particle->lightColor.Set(RE_MATH->RandomF(), RE_MATH->RandomF(), RE_MATH->RandomF());
	else
		particle->lightColor = lightColor;
	particle->intensity = (randomIntensity) ? RE_MATH->RandomF(iClamp[0], iClamp[1]) : intensity;
	particle->specular = (randomSpecular) ? RE_MATH->RandomF(sClamp[0], sClamp[1]) : specular;
}

ParticleManager::ParticleManager()
{
}

ParticleManager::~ParticleManager()
{
	simulations.clear();
}

unsigned int ParticleManager::Allocate(RE_ParticleEmitter* emitter)
{
	simulations.push_back(new eastl::pair<RE_ParticleEmitter*, eastl::list<RE_Particle*>*>(emitter, new eastl::list<RE_Particle*>()));
	return (emitter->id = ++emitter_count);
}

bool ParticleManager::Deallocate(unsigned int index)
{
	eastl::list<eastl::pair<RE_ParticleEmitter*, eastl::list<RE_Particle*>*>*>::iterator it;
	for (it = simulations.begin(); it != simulations.end(); ++it)
	{
		if ((*it)->first->id == index)
		{
			simulations.erase(it);
			return true;
		}
	}

	return false;
}

bool ParticleManager::SetEmitterState(unsigned int index, RE_ParticleEmitter::PlaybackState state)
{
	eastl::list<eastl::pair<RE_ParticleEmitter*, eastl::list<RE_Particle*>*>*>::iterator it;
	for (it = simulations.begin(); it != simulations.end(); ++it)
	{
		if ((*it)->first->id == index)
		{
			(*it)->first->state = state;
			return true;
		}
	}

	return false;
}
