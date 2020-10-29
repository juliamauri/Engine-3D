#ifndef __RE_COMPLIGHT_H__
#define __RE_COMPLIGHT_H__

#include "RE_Component.h"
#include "MathGeoLib/include/Math/float3.h"

enum LightType : int
{
	L_DIRECTIONAL = 0,
	L_POINT,
	L_SPOTLIGHT
};

class RE_CompLight : public RE_Component
{
public:

	RE_CompLight();
	~RE_CompLight();

	void SetUp(RE_GameObject* parent);
	void SetUp(const RE_CompLight& cmpLight, RE_GameObject* parent);

	void CallShaderUniforms(unsigned int shader, const char* unif_name) const;

	void DrawProperties() override;

	void SerializeJson(JSONNode* node, eastl::map<const char*, int>* resources) override;
	void DeserializeJson(JSONNode* node, eastl::map<int, const char*>* resources, RE_GameObject* parent) override;

	unsigned int GetBinarySize()const override;
	void SerializeBinary(char*& cursor, eastl::map<const char*, int>* resources) override;
	void DeserializeBinary(char*& cursor, eastl::map<int, const char*>* resources, RE_GameObject* parent)override;

private:

	inline void UpdateCutOff();

public:

	LightType type = L_POINT;

	// Attenuattion
	float intensity = 1.0f;
	float constant = 1.0f;
	float linear = 0.091f;
	float quadratic = 0.011f;

	// color
	math::vec diffuse; // 0.8
	float specular = 0.2f;

	// Spotlight
	float cutOff[2]; // cos(radians(12.5f))
	float outerCutOff[2]; // cos(radians(17.5f))
};

#endif // !__RE_COMPLIGHT_H__