#include "RE_CompTransform.h"

#include "RE_GameObject.h"
#include "ImGui\imgui.h"
#include "RE_Math.h"

#define MIN_SCALE 0.001f

RE_CompTransform::RE_CompTransform(RE_GameObject * go) : RE_Component(C_TRANSFORM, go) 
{
	scale.scale = math::vec::one;
	if (go == nullptr)
		useParent = false;
}

RE_CompTransform::~RE_CompTransform()
{
}

void RE_CompTransform::Update()
{
	if (needed_update_transform)
		CalcGlobalTransform();
}

math::float4x4 RE_CompTransform::GetMatrixModel()
{
	return model_global;
}

float* RE_CompTransform::GetShaderModel()
{
	return model_global.ptr();
}

void RE_CompTransform::SetRotation(math::Quat rotation)
{
	rot_quat = rotation;
	rot_eul = rotation.ToEulerXYZ();
	rot_mat = rotation.ToFloat3x3();
	needed_update_transform = true;
}

void RE_CompTransform::SetRotation(math::vec rotation)
{
	rot_eul = rotation;
	rot_quat = math::Quat::FromEulerXYZ(rotation.x, rotation.y, rotation.z);
	rot_mat = math::float3x3::FromEulerXYZ(rotation.x, rotation.y, rotation.z);
	needed_update_transform = true;
}

void RE_CompTransform::SetRotation(math::float3x3 rotation)
{
	rot_mat = rotation;
	rot_eul = rotation.ToEulerXYZ();
	rot_quat = rotation.ToQuat();
	needed_update_transform = true;
}

void RE_CompTransform::SetScale(math::vec scale)
{
	this->scale.scale = scale;
	this->scale.scale = {
	this->scale.scale.x > MIN_SCALE ? 1.0f / this->scale.scale.x : 1.0f / MIN_SCALE,
	this->scale.scale.y > MIN_SCALE ? 1.0f / this->scale.scale.y : 1.0f / MIN_SCALE,
	this->scale.scale.z > MIN_SCALE ? 1.0f / this->scale.scale.z : 1.0f / MIN_SCALE };

	needed_update_transform = true;
}

void RE_CompTransform::SetPosition(math::vec position)
{
	pos = position;
	needed_update_transform = true;
}

void RE_CompTransform::SetGlobalPosition(math::vec global_position)
{
	pos = global_position - go->GetParent()->GetTransform()->GetGlobalPosition();
	needed_update_transform = true;
}

math::Quat RE_CompTransform::GetQuaternionRotation()
{
	return rot_quat;
}

math::vec RE_CompTransform::GetEulerRotation()
{
	return rot_eul;
}

math::vec RE_CompTransform::GetScale()
{
	return scale.scale;
}

math::vec RE_CompTransform::GetPosition()
{
	return pos;
}

math::vec RE_CompTransform::GetGlobalPosition()
{
	return model_global.Row3(3);
}

void RE_CompTransform::DrawProperties()
{
	if (ImGui::CollapsingHeader("Transform"))
	{
		// Position -----------------------------------------------------
		float p[3] = { pos.x, pos.y, pos.z };
		if (ImGui::DragFloat3("Position", p, 0.1f, -10000.f, 10000.f, "%.2f"))
			SetPosition({ p[0], p[1], p[2] });

		// Rotation -----------------------------------------------------
		float r[3] = { rot_eul.x, rot_eul.y, rot_eul.z };
		if (ImGui::DragFloat3("Rotation", r, 0.1f, -360.f, 360.f, "%.2f"))
			SetRotation({ r[0], r[1], r[2] });

		// Scale -----------------------------------------------------
		float s[3] = { scale.scale.x, scale.scale.y, scale.scale.z };
		if (ImGui::InputFloat3("Scale", s, 2))
			SetScale({ s[0], s[1], s[2] });
	}
}

bool RE_CompTransform::HasChanged()
{
	has_changed = false;
	return !has_changed;
}

void RE_CompTransform::CalcGlobalTransform()
{
	model_local = math::float4x4::identity;

	model_local = model_local * scale;

	model_local = model_local * math::float4x4::RotateX(math::DegToRad(rot_eul.x));
	model_local = model_local * math::float4x4::RotateY(math::DegToRad(rot_eul.y));
	model_local = model_local * math::float4x4::RotateZ(math::DegToRad(rot_eul.z));

	model_local = model_local * math::float4x4::Translate(pos);

	model_local.Transpose();

	if (useParent)
		model_global = go->GetParent()->GetTransform()->GetMatrixModel() * model_local;
	else
		model_global = model_local;

	needed_update_transform = false;
	has_changed = true;
}

