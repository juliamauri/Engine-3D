#ifndef __RE_COMPONENT_H__
#define __RE_COMPONENT_H__

#include "Globals.h"

#include <EASTL/vector.h>
#include <EASTL/map.h>

class RE_GameObject;
class JSONNode;
class GameObjectsPool;

enum ComponentType : ushortint
{
	C_EMPTY = 0x00,
	C_TRANSFORM,
	C_MESH = 15,
	C_CAMERA,
	C_LIGHT,
	C_PARTICLEEMITER,

	C_PRIMIVE_MIN = 20,
	C_GRID,
	C_CUBE,
	C_DODECAHEDRON,
	C_TETRAHEDRON,
	C_OCTOHEDRON,
	C_ICOSAHEDRON,
	C_PLANE,
	C_FUSTRUM,
	C_SPHERE,
	C_CYLINDER,
	C_HEMISHPERE,
	C_TORUS,
	C_TREFOILKNOT,
	C_ROCK,
	C_PRIMIVE_MAX,

	MAX_COMPONENT_TYPES
};

class RE_Component
{
public:
	RE_Component(const ComponentType type = C_EMPTY, const UID go = 0, const bool start_active = true) :
		type(type), go(go), active(start_active) {}
	virtual ~RE_Component() {}

	virtual UID PoolSetUp(GameObjectsPool* pool, const UID parent, bool report_parent = false);
	virtual void CopySetUp(GameObjectsPool* pool, RE_Component* copy, const UID parent) {}

	virtual void Init() {}
	virtual void CleanUp() {}

	virtual void PreUpdate() {}
	virtual void Update() {}
	virtual void PostUpdate() {}

	virtual void Draw() const {}
	virtual void DrawProperties() {}

	virtual void OnPlay() {}
	virtual void OnPause() {}
	virtual void OnStop() {}

	virtual void Save() const {}
	virtual void Load() {}
	virtual void Reset() {}

	virtual void OnTransformModified() {}

	bool IsActive() const { return active; }
	void SetActive(const bool value) { active = value; }

	ComponentType GetType() const { return type; }
	UID GetGOUID() const { return go; }
	RE_GameObject* GetGOPtr() const;
	const RE_GameObject* GetGOCPtr() const;
	void SetParent(const UID parent) { go = parent; };

	virtual eastl::vector<const char*> GetAllResources() { return eastl::vector<const char*>(); }

	virtual void SerializeJson(JSONNode* node, eastl::map<const char*, int>* resources) const {}
	virtual void DeserializeJson(JSONNode* node, eastl::map<int, const char*>* resources) {}

	virtual unsigned int GetBinarySize() const {  return 0; }
	virtual void SerializeBinary(char*& cursor, eastl::map<const char*, int>* resources) const {}
	virtual void DeserializeBinary(char*& cursor, eastl::map<int, const char*>* resources) {}

	virtual void UseResources() {  }
	virtual void UnUseResources() {  }

	//POOL
	UID GetPoolID()const { return id; }
	void SetPoolID(UID id) { id = id; }

protected:

	bool active = true;
	ComponentType type = C_EMPTY;
	UID id = 0;
	UID go = 0;
	GameObjectsPool* pool_gos = nullptr;
	bool useParent = false;
};

#endif // !__RE_COMPONENT_H__