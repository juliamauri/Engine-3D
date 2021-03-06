#include "RE_Math.h"

#ifdef _DEBUG
#pragma comment(lib, "MathGeoLib/lib/MathGeoLib_debug.lib")
#else
#pragma comment(lib, "MathGeoLib/lib/MathGeoLib_release.lib")
#endif

RE_Math::RE_Math()
{
	lcg.Seed(math::Clock::TickU32());
}

// range [ 0, 1 )
float RE_Math::RandomF() { return lcg.Float(); }

// range ( -1, 1 )
float RE_Math::RandomFN() { return (lcg.Float() * 2.f) - 1.f; }

// range [ 0, 2147483647 ]
int RE_Math::RandomInt() { return lcg.Int(); }

// @param a Lower bound, inclusive.
// @param b Upper bound, inclusive.
float RE_Math::RandomF(float min, float max) { return lcg.Float(min, max); }

// @param a Lower bound, inclusive.
// @param b Upper bound, exclusive.
int RE_Math::RandomInt(int min, int max) { return lcg.Int(min, max); }

unsigned long long RE_Math::RandomUID()
{
	unsigned long long ret = static_cast<unsigned long long>(lcg.Int());
	ret = ret << 32;
	ret += static_cast<unsigned long long>(lcg.Int()) + 1ull;

	return ret;
}

math::vec RE_Math::RandomVec()
{
	return math::vec(lcg.Float(), lcg.Float(), lcg.Float());
}

math::vec RE_Math::RandomNVec()
{
	return math::vec(lcg.Float() - 0.5f, lcg.Float() - 0.5f, lcg.Float() - 0.5f).Normalized();
}

math::LCG& RE_Math::GetRNGSeed() { return lcg; }

void RE_Math::SetRNGSeed(unsigned int seed) { lcg.Seed(seed); }

const float RE_Math::pi = 3.1415926f;
const float RE_Math::pi_x2 = 6.2831853f;
const float RE_Math::pi_div2 = 1.5707963f;
const float RE_Math::deg_to_rad = 0.01745329f;
const float RE_Math::rad_to_deg = 57.2957795f;