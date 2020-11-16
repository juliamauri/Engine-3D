#include "RE_GOManager.h"

#include "Application.h"
#include "ModuleScene.h"
#include "RE_FileSystem.h"

RE_GOManager::RE_GOManager() {}
RE_GOManager::~RE_GOManager() {}

void RE_GOManager::Update()
{
	componentsPool.Update();
}

RE_GameObject* RE_GOManager::AddGO(const char* name, UID parent, bool broadcast)
{
	if (gameObjectsPool.GetCount() > 0 && !parent)
		parent = gameObjectsPool.GetRootUID();


	UID new_go_uid = gameObjectsPool.GetNewGOUID();
	RE_GameObject* ret = gameObjectsPool.AtPtr(new_go_uid);
	ret->SetUp(&gameObjectsPool, &componentsPool, name, parent);

	Event::Push(GO_HAS_NEW_CHILD, App::scene, parent, new_go_uid);

	return ret;
}

RE_GameObject* RE_GOManager::CopyGO(const RE_GameObject* copy, UID parent, bool broadcast)
{
	RE_GameObject* new_go = AddGO(copy->name.c_str(), parent, broadcast);

	for (auto copy_comp : copy->AllCompData())
		componentsPool.CopyComponent(&gameObjectsPool, copy->pool_comps->GetComponentPtr(copy_comp.uid, static_cast<ComponentType>(copy_comp.type)), new_go->go_uid);
	
	return new_go;
}

RE_GameObject* RE_GOManager::CopyGOandChilds(const RE_GameObject* copy, UID parent, bool broadcast)
{
	RE_GameObject* ret = CopyGO(copy, parent, broadcast);
	UID ret_uid = ret->go_uid;

	eastl::stack<eastl::pair<const RE_GameObject*, UID>> copy_gos;
	for (auto copy_child : copy->GetChildsCPtr())
		copy_gos.push({ copy_child, ret_uid });

	while (!copy_gos.empty())
	{
		eastl::pair<const RE_GameObject*, UID> copy_go = copy_gos.top();
		copy_gos.pop();

		RE_GameObject* new_go = CopyGO(copy_go.first, copy_go.second, broadcast);
		UID new_go_uid = new_go->go_uid;

		for (auto copy_child : copy_go.first->GetChildsCPtr())
			copy_gos.push({ copy_child, new_go_uid });
	}

	return ret;
}

RE_GameObject* RE_GOManager::GetGOPtr(UID id) const
{
	return gameObjectsPool.AtPtr(id);
}

const RE_GameObject* RE_GOManager::GetGOCPtr(UID id) const
{
	return gameObjectsPool.AtCPtr(id);
}

UID RE_GOManager::GetRootUID() const
{
	return gameObjectsPool.GetRootUID();
}

RE_GameObject* RE_GOManager::GetRootPtr() const
{
	return gameObjectsPool.GetRootPtr();
}

const RE_GameObject* RE_GOManager::GetRootCPtr() const
{
	return gameObjectsPool.GetRootCPtr();
}

eastl::vector<UID> RE_GOManager::GetAllGOUIDs() const
{
	return gameObjectsPool.GetAllKeys();
}

eastl::vector<RE_GameObject*> RE_GOManager::GetAllGOPtrs() const
{
	return gameObjectsPool.GetAllPtrs();
}

eastl::vector<eastl::pair<UID, RE_GameObject*>> RE_GOManager::GetAllGOData() const
{
	return gameObjectsPool.GetAllData();
}

void RE_GOManager::DestroyGO(UID toDestroy)
{
	RE_GameObject* go = gameObjectsPool.AtPtr(toDestroy);
	go->UnlinkParent();

	for (auto child : go->childs) RecursiveDestroyGO(child);
	for (auto comp : go->AllCompData()) componentsPool.DestroyComponent(static_cast<ComponentType>(comp.type), comp.uid);

	gameObjectsPool.DeleteGO(toDestroy);
}

// Recursive
void RE_GOManager::RecursiveDestroyGO(UID toDestroy)
{
	RE_GameObject* go = gameObjectsPool.AtPtr(toDestroy);
	for (auto child : go->childs) RecursiveDestroyGO(child);
	for (auto comp : go->AllCompData()) componentsPool.DestroyComponent(static_cast<ComponentType>(comp.type), comp.uid);

	gameObjectsPool.DeleteGO(toDestroy);
}

RE_GameObject* RE_GOManager::InsertPool(RE_GOManager* pool)
{
	return RecusiveInsertGO(pool->GetRootPtr(), (TotalGameObjects() > 0) ? GetRootUID() : 0);
}

eastl::vector<UID> RE_GOManager::GetAllCompUID(ushortint type) const
{
	return componentsPool.GetAllCompUID(type);
}

eastl::vector<RE_Component*> RE_GOManager::GetAllCompPtr(ushortint type) const
{
	return componentsPool.GetAllCompPtr(type);
}

eastl::vector<const RE_Component*> RE_GOManager::GetAllCompCPtr(ushortint type) const
{
	return componentsPool.GetAllCompCPtr(type);
}

eastl::vector<eastl::pair<UID, RE_Component*>> RE_GOManager::GetAllCompData(ushortint type) const
{
	return componentsPool.GetAllCompData(type);
}

RE_Component* RE_GOManager::GetComponentPtr(UID poolid, ComponentType cType)
{
	return componentsPool.GetComponentPtr(poolid, cType);
}

const RE_Component* RE_GOManager::GetComponentCPtr(UID poolid, ComponentType cType) const
{
	return componentsPool.GetComponentCPtr(poolid, cType);
}

RE_GOManager* RE_GOManager::GetNewPoolFromID(UID id)
{
	RE_GOManager* ret = new RE_GOManager();

	ret->RecusiveInsertGO(gameObjectsPool.AtPtr(id), 0);

	return ret;
}

eastl::vector<const char*> RE_GOManager::GetAllResources()
{
	return componentsPool.GetAllResources();
}

void RE_GOManager::UseResources()
{
	componentsPool.UseResources();
}

void RE_GOManager::UnUseResources()
{
	componentsPool.UnUseResources();
}

void RE_GOManager::ClearPool()
{
	gameObjectsPool.Clear();
	componentsPool.ClearComponents();
}

unsigned int RE_GOManager::GetBinarySize() const
{
	return gameObjectsPool.GetBinarySize() + componentsPool.GetBinarySize();
}

void RE_GOManager::SerializeJson(JSONNode* node, eastl::map<const char*, int>* resources)
{
	gameObjectsPool.SerializeJson(node);
	componentsPool.SerializeJson(node, resources);
}

void RE_GOManager::DeserializeJson(JSONNode* node, eastl::map<int, const char*>* resources)
{
	gameObjectsPool.DeserializeJson(node, &componentsPool);
	componentsPool.DeserializeJson(&gameObjectsPool, node, resources);
}

void RE_GOManager::SerializeBinary(char*& cursor, eastl::map<const char*, int>* resources)
{
	gameObjectsPool.SerializeBinary(cursor);
	componentsPool.SerializeBinary(cursor, resources);
}

void RE_GOManager::DeserializeBinary(char*& cursor, eastl::map<int, const char*>* resources)
{
	gameObjectsPool.DeserializeBinary(cursor, &componentsPool);
	componentsPool.DeserializeBinary(&gameObjectsPool, cursor, resources);
}

RE_GameObject* RE_GOManager::RecusiveInsertGO(RE_GameObject* go, UID parent)
{
	RE_GameObject* ret = CopyGO(go, parent);

	eastl::list<RE_GameObject*> childs = go->GetChildsPtr();
	for (RE_GameObject* c : childs)
			RecusiveInsertGO(c, ret->go_uid);

	return ret;
}

void GameObjectsPool::Clear()
{
	poolmapped_.clear();
	lastAvaibleIndex = 0;
}

UID GameObjectsPool::Push(RE_GameObject val)
{
	UID ret = 0;
	if (PoolMapped::Push(val, val.go_uid = RE_Math::RandomUID())) ret = val.go_uid;
	return ret;
}

UID GameObjectsPool::GetNewGOUID()
{
	return Push({});
}

RE_GameObject* GameObjectsPool::GetNewGOPtr()
{
	return AtPtr(Push({}));
}

UID GameObjectsPool::GetRootUID() const
{
	return (lastAvaibleIndex > 0) ? pool_[0].go_uid : 0;
}

RE_GameObject* GameObjectsPool::GetRootPtr() const
{
	return (lastAvaibleIndex > 0) ? &pool_[0] : nullptr;
}

const RE_GameObject* GameObjectsPool::GetRootCPtr() const
{
	return (lastAvaibleIndex > 0) ? &pool_[0] : nullptr;
}

void GameObjectsPool::DeleteGO(UID toDelete)
{
	PoolMapped::Pop(toDelete);
}

eastl::vector<RE_GameObject*> GameObjectsPool::GetAllPtrs() const
{
	eastl::vector<RE_GameObject*> ret;
	eastl::vector<UID> uids = GetAllKeys();
	for (auto uid : uids) ret.push_back(AtPtr(uid));
	return ret;
}

eastl::vector<eastl::pair<UID, RE_GameObject*>> GameObjectsPool::GetAllData() const
{
	eastl::vector<eastl::pair<UID, RE_GameObject*>> ret;
	eastl::vector<UID> uids = GetAllKeys();
	for (auto uid : uids) ret.push_back({ uid, AtPtr(uid) });
	return ret;
}

unsigned int GameObjectsPool::GetBinarySize() const
{
	uint size = sizeof(unsigned int);
	size += lastAvaibleIndex * sizeof(UID);
	for (int i = 0; i < lastAvaibleIndex; i++)
		size += pool_[i].GetBinarySize();
	return size;
}

void GameObjectsPool::SerializeBinary(char*& cursor)
{
	uint size = sizeof(unsigned int);
	uint goSize = GetCount();
	memcpy(cursor, &goSize, size);
	cursor += size;

	size = sizeof(UID);
	for (uint i = 0; i < goSize; i++)
	{
		memcpy(cursor, &pool_[i].go_uid, size);
		cursor += size;
		pool_[i].SerializeBinary(cursor);
	}
}

void GameObjectsPool::DeserializeBinary(char*& cursor, ComponentsPool* cmpsPool)
{
	eastl::map<UID, RE_GameObject*> idGO;

	uint size = sizeof(unsigned int);
	uint goSize;
	memcpy(&goSize, cursor, size);
	cursor += size;

	size = sizeof(UID);
	for (uint i = 0; i < goSize; i++)
	{
		RE_GameObject newGO; UID goUID;
		memcpy(&goUID, cursor, size);
		cursor += size;
		newGO.go_uid = goUID;
		PoolMapped::Push(newGO, goUID);
		AtPtr(goUID)->DeserializeBinary(cursor, this, cmpsPool);
	}
}

void GameObjectsPool::SerializeJson(JSONNode* node)
{
	JSONNode* goPool = node->PushJObject("gameobjects Pool");
	uint goSize = GetCount();
	goPool->PushUInt("gameobjectsSize", goSize);
	for (uint i = 0; i < goSize; i++)
	{
		JSONNode* goNode = goPool->PushJObject(eastl::to_string(i).c_str());
		goNode->PushUnsignedLongLong("GOUID", pool_[i].go_uid);
		pool_[i].SerializeJson(goNode);
		DEL(goNode);
	}
	DEL(goPool);
}

void GameObjectsPool::DeserializeJson(JSONNode* node, ComponentsPool* cmpsPool)
{
	eastl::map<UID, RE_GameObject*> idGO;
	JSONNode* goPool = node->PullJObject("gameobjects Pool");
	unsigned int goSize = goPool->PullUInt("gameobjectsSize",  0);

	for (uint i = 0; i < goSize; i++) {
		JSONNode* goNode = goPool->PullJObject(eastl::to_string(i).c_str());
		UID goUID = goNode->PullUnsignedLongLong("GOUID", 0);
		RE_GameObject newGO;
		newGO.go_uid = goUID;
		PoolMapped::Push(newGO, goUID);
		AtPtr(goUID)->DeserializeJSON(goNode, this, cmpsPool);
		DEL(goNode);
	}
	DEL(goPool);
}

eastl::vector<UID> GameObjectsPool::GetAllKeys() const
{
	eastl::vector<UID> ret;
	for (auto go : poolmapped_) ret.push_back(go.first);
	return ret;
}

void ComponentsPool::Update()
{
	transPool.Update();
	camPool.Update();
}

void ComponentsPool::ClearComponents()
{
	transPool.Clear();
	camPool.Clear();
	meshPool.Clear();
	lightPool.Clear();
	pGridPool.Clear();
	pRockPool.Clear();
	pCubePool.Clear();
	pDodecahedronPool.Clear();
	pTetrahedronPool.Clear();
	pOctohedronPool.Clear();
	pIcosahedronPool.Clear();
	pPlanePool.Clear();
	pSpherePool.Clear();
	pCylinderPool.Clear();
	pHemiSpherePool.Clear();
	pTorusPool.Clear();
	pTrefoiKnotPool.Clear();
}

RE_Component* ComponentsPool::GetComponentPtr(UID poolid, ComponentType cType)
{
	RE_Component* ret = nullptr;
	switch (cType) {
	case C_TRANSFORM: ret = static_cast<RE_Component*>(transPool.AtPtr(poolid)); break;
	case C_CAMERA: ret = static_cast<RE_Component*>(camPool.AtPtr(poolid)); break;
	case C_MESH: ret = static_cast<RE_Component*>(meshPool.AtPtr(poolid)); break;
	case C_LIGHT: ret = static_cast<RE_Component*>(lightPool.AtPtr(poolid)); break;
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID: ret = static_cast<RE_Component*>(pGridPool.AtPtr(poolid)); break;
	case C_ROCK: ret = static_cast<RE_Component*>(pRockPool.AtPtr(poolid)); break;
	case C_CUBE: ret = static_cast<RE_Component*>(pCubePool.AtPtr(poolid)); break;
	case C_DODECAHEDRON: ret = static_cast<RE_Component*>(pDodecahedronPool.AtPtr(poolid)); break;
	case C_TETRAHEDRON: ret = static_cast<RE_Component*>(pTetrahedronPool.AtPtr(poolid)); break;
	case C_OCTOHEDRON: ret = static_cast<RE_Component*>(pOctohedronPool.AtPtr(poolid)); break;
	case C_ICOSAHEDRON: ret = static_cast<RE_Component*>(pIcosahedronPool.AtPtr(poolid)); break;
	case C_PLANE: ret = static_cast<RE_Component*>(pPlanePool.AtPtr(poolid)); break;
	case C_SPHERE: ret = static_cast<RE_Component*>(pSpherePool.AtPtr(poolid)); break;
	case C_CYLINDER: ret = static_cast<RE_Component*>(pCylinderPool.AtPtr(poolid)); break;
	case C_HEMISHPERE: ret = static_cast<RE_Component*>(pHemiSpherePool.AtPtr(poolid)); break;
	case C_TORUS: ret = static_cast<RE_Component*>(pTorusPool.AtPtr(poolid)); break;
	case C_TREFOILKNOT:  ret = static_cast<RE_Component*>(pTrefoiKnotPool.AtPtr(poolid)); break;}
	return ret;
}

const RE_Component* ComponentsPool::GetComponentCPtr(UID poolid, ComponentType cType) const
{
	const RE_Component* ret = nullptr;
	switch (cType) {
	case C_TRANSFORM: ret = static_cast<const RE_Component*>(transPool.AtCPtr(poolid)); break;
	case C_CAMERA: ret = static_cast<const RE_Component*>(camPool.AtCPtr(poolid)); break;
	case C_MESH: ret = static_cast<const RE_Component*>(meshPool.AtCPtr(poolid)); break;
	case C_LIGHT: ret = static_cast<const RE_Component*>(lightPool.AtCPtr(poolid)); break;
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM:  break;
	case C_GRID: ret = static_cast<const RE_Component*>(pGridPool.AtCPtr(poolid)); break;
	case C_ROCK: ret = static_cast<const RE_Component*>(pRockPool.AtCPtr(poolid)); break;
	case C_CUBE: ret = static_cast<const RE_Component*>(pCubePool.AtCPtr(poolid)); break;
	case C_DODECAHEDRON: ret = static_cast<const RE_Component*>(pDodecahedronPool.AtCPtr(poolid)); break;
	case C_TETRAHEDRON: ret = static_cast<const RE_Component*>(pTetrahedronPool.AtCPtr(poolid)); break;
	case C_OCTOHEDRON: ret = static_cast<const RE_Component*>(pOctohedronPool.AtCPtr(poolid)); break;
	case C_ICOSAHEDRON: ret = static_cast<const RE_Component*>(pIcosahedronPool.AtCPtr(poolid)); break;
	case C_PLANE: ret = static_cast<const RE_Component*>(pPlanePool.AtCPtr(poolid)); break;
	case C_SPHERE: ret = static_cast<const RE_Component*>(pSpherePool.AtCPtr(poolid)); break;
	case C_CYLINDER: ret = static_cast<const RE_Component*>(pCylinderPool.AtCPtr(poolid)); break;
	case C_HEMISHPERE: ret = static_cast<const RE_Component*>(pHemiSpherePool.AtCPtr(poolid)); break;
	case C_TORUS: ret = static_cast<const RE_Component*>(pTorusPool.AtCPtr(poolid)); break;
	case C_TREFOILKNOT: ret = static_cast<const RE_Component*>(pTrefoiKnotPool.AtCPtr(poolid)); break;}
	return ret;
}

eastl::pair<UID, RE_Component*> ComponentsPool::GetNewComponent(ComponentType cType)
{
	eastl::pair<UID, RE_Component*> ret = { 0, nullptr};
	switch (cType)
	{
	case C_TRANSFORM:
	{
		ret.first = transPool.GetNewCompUID();
		ret.second = transPool.AtPtr(ret.first);
		break;
	}
	case C_CAMERA:
	{
		ret.first = camPool.GetNewCompUID();
		ret.second = camPool.AtPtr(ret.first);
		break;
	}
	case C_MESH:
	{
		ret.first = meshPool.GetNewCompUID();
		ret.second = meshPool.AtPtr(ret.first);
		break;
	}
	case C_LIGHT:
	{
		ret.first = lightPool.GetNewCompUID();
		ret.second = lightPool.AtPtr(ret.first);
		break;
	}
	case C_GRID:
	{
		ret.first = pGridPool.GetNewCompUID();
		ret.second = pGridPool.AtPtr(ret.first);
		break;
	}
	case C_ROCK:
	{
		ret.first = pRockPool.GetNewCompUID();
		ret.second = pRockPool.AtPtr(ret.first);
		break;
	}
	case C_CUBE:
	{
		ret.first = pCubePool.GetNewCompUID();
		ret.second = pCubePool.AtPtr(ret.first);
		break;
	}
	case C_DODECAHEDRON:
	{
		ret.first = pDodecahedronPool.GetNewCompUID();
		ret.second = pDodecahedronPool.AtPtr(ret.first);
		break;
	}
	case C_TETRAHEDRON:
	{
		ret.first = pTetrahedronPool.GetNewCompUID();
		ret.second = pTetrahedronPool.AtPtr(ret.first);
		break;
	}
	case C_OCTOHEDRON:
	{
		ret.first = pOctohedronPool.GetNewCompUID();
		ret.second = pOctohedronPool.AtPtr(ret.first);
		break;
	}
	case C_ICOSAHEDRON:
	{
		ret.first = pIcosahedronPool.GetNewCompUID();
		ret.second = pIcosahedronPool.AtPtr(ret.first);
		break;
	}
	case C_PLANE:
	{
		ret.first = pPlanePool.GetNewCompUID();
		ret.second = pPlanePool.AtPtr(ret.first);
		break;
	}
	case C_SPHERE:
	{
		ret.first = pSpherePool.GetNewCompUID();
		ret.second = pSpherePool.AtPtr(ret.first);
		break;
	}
	case C_CYLINDER:
	{
		ret.first = pCylinderPool.GetNewCompUID();
		ret.second = pCylinderPool.AtPtr(ret.first);
		break;
	}
	case C_HEMISHPERE:
	{
		ret.first = pHemiSpherePool.GetNewCompUID();
		ret.second = pHemiSpherePool.AtPtr(ret.first);
		break;
	}
	case C_TORUS:
	{
		ret.first = pTorusPool.GetNewCompUID();
		ret.second = pTorusPool.AtPtr(ret.first);
		break;
	}
	case C_TREFOILKNOT:
	{
		ret.first = pTrefoiKnotPool.GetNewCompUID();
		ret.second = pTrefoiKnotPool.AtPtr(ret.first);
		break;
	}
	}

	return ret;
}

UID ComponentsPool::GetNewComponentUID(ComponentType cType)
{
	UID ret = 0;
	switch (cType) {
	case C_TRANSFORM: ret = transPool.GetNewCompUID(); break;
	case C_CAMERA: ret = camPool.GetNewCompUID(); break;
	case C_MESH: ret = meshPool.GetNewCompUID(); break;
	case C_LIGHT: ret = lightPool.GetNewCompUID(); break;
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID: ret = pGridPool.GetNewCompUID(); break;
	case C_ROCK: ret = pRockPool.GetNewCompUID(); break;
	case C_CUBE: ret = pCubePool.GetNewCompUID(); break;
	case C_DODECAHEDRON: ret = pDodecahedronPool.GetNewCompUID(); break;
	case C_TETRAHEDRON: ret = pTetrahedronPool.GetNewCompUID(); break;
	case C_OCTOHEDRON: ret = pOctohedronPool.GetNewCompUID(); break;
	case C_ICOSAHEDRON:  ret = pIcosahedronPool.GetNewCompUID(); break;
	case C_PLANE: ret = pPlanePool.GetNewCompUID(); break;
	case C_SPHERE: ret = pSpherePool.GetNewCompUID(); break;
	case C_CYLINDER: ret = pCylinderPool.GetNewCompUID(); break;
	case C_HEMISHPERE: ret = pHemiSpherePool.GetNewCompUID(); break;
	case C_TORUS:  ret = pTorusPool.GetNewCompUID(); break;
	case C_TREFOILKNOT: ret = pTrefoiKnotPool.GetNewCompUID(); break;}
	return ret;
}

RE_Component* ComponentsPool::GetNewComponentPtr(ComponentType cType)
{
	RE_Component* ret = nullptr;
	switch (cType) {
	case C_TRANSFORM: ret = static_cast<RE_Component*>(transPool.AtPtr(transPool.GetNewCompUID())); break;
	case C_CAMERA: ret = static_cast<RE_Component*>(camPool.AtPtr(camPool.GetNewCompUID())); break;
	case C_MESH: ret = static_cast<RE_Component*>(meshPool.AtPtr(meshPool.GetNewCompUID())); break;
	case C_LIGHT: ret = static_cast<RE_Component*>(lightPool.AtPtr(lightPool.GetNewCompUID())); break;
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID: ret = static_cast<RE_Component*>(pGridPool.AtPtr(pGridPool.GetNewCompUID())); break;
	case C_ROCK: ret = static_cast<RE_Component*>(pRockPool.AtPtr(pRockPool.GetNewCompUID())); break;
	case C_CUBE: ret = static_cast<RE_Component*>(pCubePool.AtPtr(pCubePool.GetNewCompUID())); break;
	case C_DODECAHEDRON: ret = static_cast<RE_Component*>(pDodecahedronPool.AtPtr(pDodecahedronPool.GetNewCompUID())); break;
	case C_TETRAHEDRON: ret = static_cast<RE_Component*>(pTetrahedronPool.AtPtr(pTetrahedronPool.GetNewCompUID())); break;
	case C_OCTOHEDRON: ret = static_cast<RE_Component*>(pOctohedronPool.AtPtr(pOctohedronPool.GetNewCompUID())); break;
	case C_ICOSAHEDRON: ret = static_cast<RE_Component*>(pIcosahedronPool.AtPtr(pIcosahedronPool.GetNewCompUID())); break;
	case C_PLANE: ret = static_cast<RE_Component*>(pPlanePool.AtPtr(pPlanePool.GetNewCompUID())); break;
	case C_SPHERE: ret = static_cast<RE_Component*>(pSpherePool.AtPtr(pSpherePool.GetNewCompUID())); break;
	case C_CYLINDER: ret = static_cast<RE_Component*>(pCylinderPool.AtPtr(pCylinderPool.GetNewCompUID())); break;
	case C_HEMISHPERE: ret = static_cast<RE_Component*>(pHemiSpherePool.AtPtr(pHemiSpherePool.GetNewCompUID())); break;
	case C_TORUS: ret = static_cast<RE_Component*>(pTorusPool.AtPtr(pTorusPool.GetNewCompUID())); break;
	case C_TREFOILKNOT: ret = static_cast<RE_Component*>(pTrefoiKnotPool.AtPtr(pTrefoiKnotPool.GetNewCompUID())); break;
	}
	return ret;
}

RE_Component* ComponentsPool::CopyComponent(GameObjectsPool* pool, RE_Component* copy, UID parent)
{
	RE_Component* ret = nullptr;
	ComponentType cType = copy->GetType();
	switch (cType) {
	case C_TRANSFORM: ret = static_cast<RE_Component*>(transPool.AtPtr(transPool.GetNewCompUID())); break;
	case C_CAMERA: ret = static_cast<RE_Component*>(camPool.AtPtr(camPool.GetNewCompUID())); break;
	case C_MESH: ret = static_cast<RE_Component*>(meshPool.AtPtr(meshPool.GetNewCompUID())); break;
	case C_LIGHT: ret = static_cast<RE_Component*>(lightPool.AtPtr(lightPool.GetNewCompUID())); break;
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID: ret = static_cast<RE_Component*>(pGridPool.AtPtr(pGridPool.GetNewCompUID())); break;
	case C_ROCK: ret = static_cast<RE_Component*>(pRockPool.AtPtr(pRockPool.GetNewCompUID())); break;
	case C_CUBE: ret = static_cast<RE_Component*>(pCubePool.AtPtr(pCubePool.GetNewCompUID())); break;
	case C_DODECAHEDRON: ret = static_cast<RE_Component*>(pDodecahedronPool.AtPtr(pDodecahedronPool.GetNewCompUID())); break;
	case C_TETRAHEDRON: ret = static_cast<RE_Component*>(pTetrahedronPool.AtPtr(pTetrahedronPool.GetNewCompUID())); break;
	case C_OCTOHEDRON: ret = static_cast<RE_Component*>(pOctohedronPool.AtPtr(pOctohedronPool.GetNewCompUID())); break;
	case C_ICOSAHEDRON: ret = static_cast<RE_Component*>(pIcosahedronPool.AtPtr(pIcosahedronPool.GetNewCompUID())); break;
	case C_PLANE: ret = static_cast<RE_Component*>(pPlanePool.AtPtr(pPlanePool.GetNewCompUID())); break;
	case C_SPHERE: ret = static_cast<RE_Component*>(pSpherePool.AtPtr(pSpherePool.GetNewCompUID())); break;
	case C_CYLINDER: ret = static_cast<RE_Component*>(pCylinderPool.AtPtr(pCylinderPool.GetNewCompUID())); break;
	case C_HEMISHPERE: ret = static_cast<RE_Component*>(pHemiSpherePool.AtPtr(pHemiSpherePool.GetNewCompUID())); break;
	case C_TORUS: ret = static_cast<RE_Component*>(pTorusPool.AtPtr(pTorusPool.GetNewCompUID())); break;
	case C_TREFOILKNOT: ret = static_cast<RE_Component*>(pTrefoiKnotPool.AtPtr(pTrefoiKnotPool.GetNewCompUID())); break;
	}
	ret->CopySetUp(pool, copy, parent);
	return ret;
}

void ComponentsPool::DestroyComponent(ComponentType cType, UID toDelete)
{
	RE_Component* cmp = GetComponentPtr(toDelete, cType);
	cmp->GetGOPtr()->ReleaseComponent(toDelete, cType);
	switch (cType) {
	case C_TRANSFORM: transPool.Pop(toDelete); break;
	case C_CAMERA: camPool.Pop(toDelete); break;
	case C_MESH: meshPool.Pop(toDelete); break;
	case C_LIGHT: lightPool.Pop(toDelete); break;
	case C_GRID: pGridPool.Pop(toDelete); break;
	case C_ROCK: pRockPool.Pop(toDelete); break;
	case C_CUBE: pCubePool.Pop(toDelete); break;
	case C_DODECAHEDRON: pDodecahedronPool.Pop(toDelete); break;
	case C_TETRAHEDRON: pTetrahedronPool.Pop(toDelete); break;
	case C_OCTOHEDRON: pOctohedronPool.Pop(toDelete); break;
	case C_ICOSAHEDRON: pIcosahedronPool.Pop(toDelete); break;
	case C_PLANE:  pPlanePool.Pop(toDelete); break;
	case C_SPHERE: pSpherePool.Pop(toDelete); break;
	case C_CYLINDER: pCylinderPool.Pop(toDelete); break;
	case C_HEMISHPERE: pHemiSpherePool.Pop(toDelete); break;
	case C_TORUS: pTorusPool.Pop(toDelete); break;
	case C_TREFOILKNOT: pTrefoiKnotPool.Pop(toDelete); break;
	}
}

eastl::vector<const char*> ComponentsPool::GetAllResources()
{
	eastl::vector<const char*> allResources;
	eastl::vector<const char*> resources = camPool.GetAllResources();
	if (!resources.empty()) allResources.insert(allResources.end(), resources.begin(), resources.end());
	resources = meshPool.GetAllResources();
	if (!resources.empty()) allResources.insert(allResources.end(), resources.begin(), resources.end());

	eastl::vector<const char*> ret;
	int resSize = 0;
	for (auto res : allResources) {
		bool repeat = false;
		for (auto uniqueRes : ret) {
			resSize = eastl::CharStrlen(res);
			if (resSize > 0 && eastl::Compare(res, uniqueRes, resSize) == 0) {
				repeat = true;
				break;
			}
		}
		if (!repeat) ret.push_back(res);
	}

	return ret;
}

void ComponentsPool::UseResources()
{
	camPool.UseResources();
	meshPool.UseResources();
}

void ComponentsPool::UnUseResources()
{
	camPool.UnUseResources();
	meshPool.UnUseResources();
}

eastl::vector<UID> ComponentsPool::GetAllCompUID(ushortint type) const
{
	eastl::vector<UID> ret;
	switch (type) {
	case C_TRANSFORM: ret = transPool.GetAllKeys(); break;
	case C_CAMERA: ret = camPool.GetAllKeys(); break;
	case C_MESH: ret = meshPool.GetAllKeys(); break;
	case C_LIGHT: ret = lightPool.GetAllKeys(); break;
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID: ret = pGridPool.GetAllKeys(); break;
	case C_ROCK: ret = pRockPool.GetAllKeys(); break;
	case C_CUBE: ret = pCubePool.GetAllKeys(); break;
	case C_DODECAHEDRON: ret = pDodecahedronPool.GetAllKeys(); break;
	case C_TETRAHEDRON: ret = pTetrahedronPool.GetAllKeys(); break;
	case C_OCTOHEDRON: ret = pOctohedronPool.GetAllKeys(); break;
	case C_ICOSAHEDRON: ret = pIcosahedronPool.GetAllKeys(); break;
	case C_PLANE: ret = pPlanePool.GetAllKeys(); break;
	case C_SPHERE: ret = pSpherePool.GetAllKeys(); break;
	case C_CYLINDER: ret = pCylinderPool.GetAllKeys(); break;
	case C_HEMISHPERE: ret = pHemiSpherePool.GetAllKeys(); break;
	case C_TORUS: ret = pTorusPool.GetAllKeys(); break;
	case C_TREFOILKNOT: ret = pTrefoiKnotPool.GetAllKeys(); break;
	}

	return ret;
}

eastl::vector<RE_Component*> ComponentsPool::GetAllCompPtr(ushortint type) const
{
	eastl::vector<RE_Component*> ret;
	switch (type)
	{
	case C_TRANSFORM:
	{
		eastl::vector<UID> ids = transPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(transPool.AtPtr(id)));
		break;
	}
	case C_CAMERA:
	{
		eastl::vector<UID> ids = camPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(camPool.AtPtr(id)));
		break;
	}
	case C_MESH:
	{
		eastl::vector<UID> ids = meshPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(meshPool.AtPtr(id)));
		break;
	}
	case C_LIGHT:
	{
		eastl::vector<UID> ids = lightPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(lightPool.AtPtr(id)));
		break;
	}
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID:
	{
		eastl::vector<UID> ids = pGridPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pGridPool.AtPtr(id)));
		break;
	}
	case C_ROCK:
	{
		eastl::vector<UID> ids = pRockPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pRockPool.AtPtr(id)));
		break;
	}
	case C_CUBE:
	{
		eastl::vector<UID> ids = pCubePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pCubePool.AtPtr(id)));
		break;
	}
	case C_DODECAHEDRON:
	{
		eastl::vector<UID> ids = pDodecahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pDodecahedronPool.AtPtr(id)));
		break;
	}
	case C_TETRAHEDRON:
	{
		eastl::vector<UID> ids = pTetrahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pTetrahedronPool.AtPtr(id)));
		break;
	}
	case C_OCTOHEDRON:
	{
		eastl::vector<UID> ids = pOctohedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pOctohedronPool.AtPtr(id)));
		break;
	}
	case C_ICOSAHEDRON:
	{
		eastl::vector<UID> ids = pIcosahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pIcosahedronPool.AtPtr(id)));
		break;
	}
	case C_PLANE:
	{
		eastl::vector<UID> ids = pPlanePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pPlanePool.AtPtr(id)));
		break;
	}
	case C_SPHERE:
	{
		eastl::vector<UID> ids = pSpherePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pSpherePool.AtPtr(id)));
		break;
	}
	case C_CYLINDER:
	{
		eastl::vector<UID> ids = pCylinderPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pCylinderPool.AtPtr(id)));
		break;
	}
	case C_HEMISHPERE:
	{
		eastl::vector<UID> ids = pHemiSpherePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pHemiSpherePool.AtPtr(id)));
		break;
	}
	case C_TORUS:
	{
		eastl::vector<UID> ids = pTorusPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pTorusPool.AtPtr(id)));
		break;
	}
	case C_TREFOILKNOT:
	{
		eastl::vector<UID> ids = pTrefoiKnotPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<RE_Component*>(pTrefoiKnotPool.AtPtr(id)));
		break;
	}
	}

	return ret;
}

eastl::vector<const RE_Component*> ComponentsPool::GetAllCompCPtr(ushortint type) const
{
	eastl::vector<const RE_Component*> ret;
	switch (type)
	{
	case C_TRANSFORM:
	{
		eastl::vector<UID> ids = transPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(transPool.AtCPtr(id)));
		break;
	}
	case C_CAMERA:
	{
		eastl::vector<UID> ids = camPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(camPool.AtCPtr(id)));
		break;
	}
	case C_MESH:
	{
		eastl::vector<UID> ids = meshPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(meshPool.AtCPtr(id)));
		break;
	}
	case C_LIGHT:
	{
		eastl::vector<UID> ids = lightPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(lightPool.AtCPtr(id)));
		break;
	}
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID:
	{
		eastl::vector<UID> ids = pGridPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pGridPool.AtCPtr(id)));
		break;
	}
	case C_ROCK:
	{
		eastl::vector<UID> ids = pRockPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pRockPool.AtCPtr(id)));
		break;
	}
	case C_CUBE:
	{
		eastl::vector<UID> ids = pCubePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pCubePool.AtCPtr(id)));
		break;
	}
	case C_DODECAHEDRON:
	{
		eastl::vector<UID> ids = pDodecahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pDodecahedronPool.AtCPtr(id)));
		break;
	}
	case C_TETRAHEDRON:
	{
		eastl::vector<UID> ids = pTetrahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pTetrahedronPool.AtCPtr(id)));
		break;
	}
	case C_OCTOHEDRON:
	{
		eastl::vector<UID> ids = pOctohedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pOctohedronPool.AtCPtr(id)));
		break;
	}
	case C_ICOSAHEDRON:
	{
		eastl::vector<UID> ids = pIcosahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pIcosahedronPool.AtCPtr(id)));
		break;
	}
	case C_PLANE:
	{
		eastl::vector<UID> ids = pPlanePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pPlanePool.AtCPtr(id)));
		break;
	}
	case C_SPHERE:
	{
		eastl::vector<UID> ids = pSpherePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pSpherePool.AtCPtr(id)));
		break;
	}
	case C_CYLINDER:
	{
		eastl::vector<UID> ids = pCylinderPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pCylinderPool.AtCPtr(id)));
		break;
	}
	case C_HEMISHPERE:
	{
		eastl::vector<UID> ids = pHemiSpherePool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pHemiSpherePool.AtCPtr(id)));
		break;
	}
	case C_TORUS:
	{
		eastl::vector<UID> ids = pTorusPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pTorusPool.AtCPtr(id)));
		break;
	}
	case C_TREFOILKNOT:
	{
		eastl::vector<UID> ids = pTrefoiKnotPool.GetAllKeys();
		for (auto id : ids) ret.push_back(static_cast<const RE_Component*>(pTrefoiKnotPool.AtCPtr(id)));
		break;
	}
	}

	return ret;
}

eastl::vector<eastl::pair<UID, RE_Component*>> ComponentsPool::GetAllCompData(ushortint type) const
{
	eastl::vector<eastl::pair<UID, RE_Component*>> ret;
	switch (type)
	{
	case C_TRANSFORM:
	{
		eastl::vector<UID> ids = transPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(transPool.AtPtr(id)) });
		break;
	}
	case C_CAMERA:
	{
		eastl::vector<UID> ids = camPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(camPool.AtPtr(id)) });
		break;
	}
	case C_MESH:
	{
		eastl::vector<UID> ids = meshPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(meshPool.AtPtr(id)) });
		break;
	}
	case C_LIGHT:
	{
		eastl::vector<UID> ids = lightPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(lightPool.AtPtr(id)) });
		break;
	}
	case C_PARTICLEEMITER: break;
	case C_FUSTRUM: break;
	case C_GRID:
	{
		eastl::vector<UID> ids = pGridPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pGridPool.AtPtr(id)) });
		break;
	}
	case C_ROCK:
	{
		eastl::vector<UID> ids = pRockPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pRockPool.AtPtr(id)) });
		break;
	}
	case C_CUBE:
	{
		eastl::vector<UID> ids = pCubePool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pCubePool.AtPtr(id)) });
		break;
	}
	case C_DODECAHEDRON:
	{
		eastl::vector<UID> ids = pDodecahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pDodecahedronPool.AtPtr(id)) });
		break;
	}
	case C_TETRAHEDRON:
	{
		eastl::vector<UID> ids = pTetrahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pTetrahedronPool.AtPtr(id)) });
		break;
	}
	case C_OCTOHEDRON:
	{
		eastl::vector<UID> ids = pOctohedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pOctohedronPool.AtPtr(id)) });
		break;
	}
	case C_ICOSAHEDRON:
	{
		eastl::vector<UID> ids = pIcosahedronPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pIcosahedronPool.AtPtr(id)) });
		break;
	}
	case C_PLANE:
	{
		eastl::vector<UID> ids = pPlanePool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pPlanePool.AtPtr(id)) });
		break;
	}
	case C_SPHERE:
	{
		eastl::vector<UID> ids = pSpherePool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pSpherePool.AtPtr(id)) });
		break;
	}
	case C_CYLINDER:
	{
		eastl::vector<UID> ids = pCylinderPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pCylinderPool.AtPtr(id)) });
		break;
	}
	case C_HEMISHPERE:
	{
		eastl::vector<UID> ids = pHemiSpherePool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pHemiSpherePool.AtPtr(id)) });
		break;
	}
	case C_TORUS:
	{
		eastl::vector<UID> ids = pTorusPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pTorusPool.AtPtr(id)) });
		break;
	}
	case C_TREFOILKNOT:
	{
		eastl::vector<UID> ids = pTrefoiKnotPool.GetAllKeys();
		for (auto id : ids) ret.push_back({ id, static_cast<RE_Component*>(pTrefoiKnotPool.AtPtr(id)) });
		break;
	}
	}

	return ret;
}

unsigned int ComponentsPool::GetBinarySize() const
{
	uint size = 0;
	size += camPool.GetBinarySize();
	size += meshPool.GetBinarySize();
	size += lightPool.GetBinarySize();
	size += pGridPool.GetBinarySize();
	size += pRockPool.GetBinarySize();
	size += pCubePool.GetBinarySize();
	size += pDodecahedronPool.GetBinarySize();
	size += pTetrahedronPool.GetBinarySize();
	size += pOctohedronPool.GetBinarySize();
	size += pIcosahedronPool.GetBinarySize();
	size += pPlanePool.GetBinarySize();
	size += pSpherePool.GetBinarySize();
	size += pCylinderPool.GetBinarySize();
	size += pHemiSpherePool.GetBinarySize();
	size += pTorusPool.GetBinarySize();
	size += pTrefoiKnotPool.GetBinarySize();
	return size;
}

void ComponentsPool::SerializeBinary(char*& cursor, eastl::map<const char*, int>* resources)
{
	camPool.SerializeBinary(cursor, resources);
	meshPool.SerializeBinary(cursor, resources);
	lightPool.SerializeBinary(cursor, resources);
	pGridPool.SerializeBinary(cursor, resources);
	pRockPool.SerializeBinary(cursor, resources);
	pCubePool.SerializeBinary(cursor, resources);
	pDodecahedronPool.SerializeBinary(cursor, resources);
	pTetrahedronPool.SerializeBinary(cursor, resources);
	pOctohedronPool.SerializeBinary(cursor, resources);
	pIcosahedronPool.SerializeBinary(cursor, resources);
	pPlanePool.SerializeBinary(cursor, resources);
	pSpherePool.SerializeBinary(cursor, resources);
	pCylinderPool.SerializeBinary(cursor, resources);
	pHemiSpherePool.SerializeBinary(cursor, resources);
	pTorusPool.SerializeBinary(cursor, resources);
	pTrefoiKnotPool.SerializeBinary(cursor, resources);
}

void ComponentsPool::DeserializeBinary(GameObjectsPool* goPool, char*& cursor, eastl::map<int, const char*>* resources)
{
	camPool.DeserializeBinary(goPool, cursor, resources);
	meshPool.DeserializeBinary(goPool, cursor, resources);
	lightPool.DeserializeBinary(goPool, cursor, resources);
	pGridPool.DeserializeBinary(goPool, cursor, resources);
	pRockPool.DeserializeBinary(goPool, cursor, resources);
	pCubePool.DeserializeBinary(goPool, cursor, resources);
	pDodecahedronPool.DeserializeBinary(goPool, cursor, resources);
	pTetrahedronPool.DeserializeBinary(goPool, cursor, resources);
	pOctohedronPool.DeserializeBinary(goPool, cursor, resources);
	pIcosahedronPool.DeserializeBinary(goPool, cursor, resources);
	pPlanePool.DeserializeBinary(goPool, cursor, resources);
	pSpherePool.DeserializeBinary(goPool, cursor, resources);
	pCylinderPool.DeserializeBinary(goPool, cursor, resources);
	pHemiSpherePool.DeserializeBinary(goPool, cursor, resources);
	pTorusPool.DeserializeBinary(goPool, cursor, resources);
	pTrefoiKnotPool.DeserializeBinary(goPool, cursor, resources);
}

void ComponentsPool::SerializeJson(JSONNode* node, eastl::map<const char*, int>* resources)
{
	JSONNode* comps = node->PushJObject("Components Pool");
	camPool.SerializeJson(comps, resources);
	meshPool.SerializeJson(comps, resources);
	lightPool.SerializeJson(comps, resources);
	pGridPool.SerializeJson(comps, resources);
	pRockPool.SerializeJson(comps, resources);
	pCubePool.SerializeJson(comps, resources);
	pDodecahedronPool.SerializeJson(comps, resources);
	pTetrahedronPool.SerializeJson(comps, resources);
	pOctohedronPool.SerializeJson(comps, resources);
	pIcosahedronPool.SerializeJson(comps, resources);
	pPlanePool.SerializeJson(comps, resources);
	pSpherePool.SerializeJson(comps, resources);
	pCylinderPool.SerializeJson(comps, resources);
	pHemiSpherePool.SerializeJson(comps, resources);
	pTorusPool.SerializeJson(comps, resources);
	pTrefoiKnotPool.SerializeJson(comps, resources);
	DEL(comps);
}

void ComponentsPool::DeserializeJson(GameObjectsPool* goPool, JSONNode* node, eastl::map<int, const char*>* resources)
{
	JSONNode* comps = node->PullJObject("Components Pool");
	camPool.DeserializeJson(goPool, comps, resources);
	meshPool.DeserializeJson(goPool, comps, resources);
	lightPool.DeserializeJson(goPool, comps, resources);
	pGridPool.DeserializeJson(goPool, comps, resources);
	pRockPool.DeserializeJson(goPool, comps, resources);
	pCubePool.DeserializeJson(goPool, comps, resources);
	pDodecahedronPool.DeserializeJson(goPool, comps, resources);
	pTetrahedronPool.DeserializeJson(goPool, comps, resources);
	pOctohedronPool.DeserializeJson(goPool, comps, resources);
	pIcosahedronPool.DeserializeJson(goPool, comps, resources);
	pPlanePool.DeserializeJson(goPool, comps, resources);
	pSpherePool.DeserializeJson(goPool, comps, resources);
	pCylinderPool.DeserializeJson(goPool, comps, resources);
	pHemiSpherePool.DeserializeJson(goPool, comps, resources);
	pTorusPool.DeserializeJson(goPool, comps, resources);
	pTrefoiKnotPool.DeserializeJson(goPool, comps, resources);
	DEL(comps);
}