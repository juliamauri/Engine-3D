#ifndef __RE_GAMEOBJECT_H__
#define __RE_GAMEOBJECT_H__

#include "Globals.h"
#include "MathGeoLib\include\MathGeoLib.h"
#include <list>

#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include <windows.h>
#include <iostream>


class RE_Component;
class RE_CompTransform;
class RE_CompMesh;
class RE_CompCamera;
class JSONNode;

class RE_GameObject
{
public:
	RE_GameObject(const char* name, UUID uuid = GUID_NULL, RE_GameObject* parent = nullptr, bool start_active = true, bool isStatic = true);
	RE_GameObject(const RE_GameObject& go, RE_GameObject* parent = nullptr);
	~RE_GameObject();

	void PreUpdate();
	void Update();
	void PostUpdate();
	void Draw(bool recursive = true);

	void Serialize(JSONNode* node);

	// Children
	void AddChild(RE_GameObject* child);
	void RemoveChild(RE_GameObject* child); //Breaks the link with the parent but does not delete the child.
	void RemoveAllChilds();
	std::list<RE_GameObject*>& GetChilds();
	const std::list<RE_GameObject*>& GetChilds() const;
	unsigned int ChildCount() const;

	// Parent
	RE_GameObject* GetParent() const;
	void SetParent(RE_GameObject* parent);

	// Active
	bool IsActive() const;
	void SetActive(const bool value);
	void SetActiveAll(const bool value);

	// Static
	bool IsStatic() const;
	void SetStatic(bool value);

	// Editor Controls
	void OnPlay();
	void OnPause();
	void OnStop();

	// Components
	void AddComponent(RE_Component* component);
	RE_Component* AddComponent(const ushortint type, const char* file_path_data = nullptr, const bool dropped = false);
	RE_Component* GetComponent(const ushortint type) const;
	void RemoveComponent(RE_Component* component);
	void RemoveAllComponents();

	RE_CompTransform* AddCompTransform();
	RE_CompMesh* AddCompMesh(const char* file_path_data = nullptr, const bool dropped = false);
	void AddCompMesh(RE_CompMesh* comp_mesh);
	RE_CompCamera* AddCompCamera();
	RE_CompCamera* AddCompCamera(bool prespective, float near_plane, float far_plane, float h_fov_rads, float v_fov_rads, float h_fov_degrees, float v_fov_degrees, math::vec position, math::vec rotation, math::vec scale);

	RE_CompTransform* GetTransform() const;
	RE_CompMesh* GetMesh() const;
	RE_CompCamera* GetCamera() const;

	RE_GameObject* GetGoFromUUID(UUID parent);

	// Transform
	void TransformModified();

	// Name
	const char* GetName() const;

	// AABB
	void DrawAABB(math::vec color);
	void DrawGlobalAABB();
	void DrawAllAABB();
	void AddToBoundingBox(math::AABB box);
	void ResetBoundingBoxFromChilds();
	void ResetGlobalBoundingBox();
	math::AABB GetLocalBoundingBox() const;
	math::AABB GetGlobalBoundingBox() const;

	// Editor
	void DrawProperties();
	void DrawHeriarchy();

private:

	bool active = true;
	bool isStatic = true;

	UUID uuid;

	std::string name;
	math::AABB local_bounding_box;
	math::AABB global_bounding_box;

	RE_GameObject* parent = nullptr;
	RE_CompTransform* transform = nullptr;

	std::list<RE_GameObject*> childs;
	std::list<RE_Component*> components;
};

#endif // !__RE_GAMEOBJECT_H__