#include "RE_ResourceManager.h"

#include "Application.h"
#include "RE_FileSystem.h"
#include "RE_TextureImporter.h"
#include "RE_ModelImporter.h"
#include "RE_ThumbnailManager.h"

#include "RE_Material.h"
#include "RE_Shader.h"
#include "RE_Prefab.h"
#include "RE_Model.h"
#include "RE_Scene.h"
#include "RE_SkyBox.h"
#include "RE_Texture.h"
#include "RE_Mesh.h"


#include "Globals.h"
#include "OutputLog.h"
#include "SDL2\include\SDL_assert.h"

#include <EASTL/string.h>
#include <EASTL/internal/char_traits.h>

RE_ResourceManager::RE_ResourceManager() {}

RE_ResourceManager::~RE_ResourceManager()
{
	while (!resources.empty())
	{
		//LOG("WARNING: Deleating Unreferenced Resource: %s (from %s)",
			//resources.begin()->second.first->GetName(),
			//resources.begin()->second.first->GetOrigin());
		DEL(resources.begin()->second);
		resources.erase(resources.begin());
	}
}

void RE_ResourceManager::RecieveEvent(const Event& e)
{
	if (e.type == RE_EventType::RESOURCE_CHANGED) {
		ResourceContainer* res = resources.at(e.data1.AsCharP());
		if (res->GetType() == Resource_Type::R_SHADER) {
			eastl::vector<ResourceContainer*> materials = GetResourcesByType(Resource_Type::R_MATERIAL);
			for (auto material : materials) {
				material->SomeResourceChanged(res->GetMD5());
			}
		}
	}
}

const char* RE_ResourceManager::Reference(ResourceContainer* rc)
{
	eastl::string resourceName;
	switch (rc->GetType())
	{
	case Resource_Type::R_TEXTURE:
		resourceName = "texture";
		break;
	case Resource_Type::R_SCENE:
		resourceName = "scene";
		break;
	case Resource_Type::R_MATERIAL:
		resourceName = "material";
		break;
	case Resource_Type::R_MESH:
		resourceName = "mesh";
		break;
	case Resource_Type::R_PREFAB:
		resourceName = "prefab";
		break;
	case Resource_Type::R_PRIMITIVE:
		resourceName = "primitive";
		break;
	case Resource_Type::R_SHADER:
		resourceName = "shader";
		break;
	case Resource_Type::R_MODEL:
		resourceName = "model";
		break;
	case Resource_Type::R_SKYBOX:
		resourceName = "skybox";
		break;
	case Resource_Type::R_UNDEFINED:
		resourceName = "undefined";
		break;
	}
	LOG("Referencing the %s %s resource from %s\nAsset file: %s\nmd5 generated: %s\n", rc->GetName(), resourceName.c_str(), rc->GetAssetPath(), rc->GetLibraryPath(), rc->GetMD5());
	resources.insert(Resource(rc->GetMD5(), rc));
	resourcesCounter.insert(ResourceCounter(rc->GetMD5(), (rc->isInMemory()) ? 1 : 0));

	return rc->GetMD5();
}

void RE_ResourceManager::Use(const char* resMD5)
{
	if (resourcesCounter.at(resMD5) == 0) resources.at(resMD5)->LoadInMemory();
	resourcesCounter.at(resMD5)++;
}

void RE_ResourceManager::UnUse(const char* resMD5)
{
	if (--resourcesCounter.at(resMD5) == 0) resources.at(resMD5)->UnloadMemory();
	else if (resourcesCounter.at(resMD5) < 0) {
		LOG_WARNING("UnUse of resource already with no uses. Resource %s.",resources.at(resMD5)->GetName());
		if(resources.at(resMD5)->isInMemory()) resources.at(resMD5)->UnloadMemory();
		resourcesCounter.at(resMD5) = 0;
	}
}

void RE_ResourceManager::PushSelected(const char* resS, bool popAll)
{
	if (popAll)PopSelected(true);
	resourcesSelected.push(resS);
}

const char* RE_ResourceManager::GetSelected() const
{
	return (resourcesSelected.empty()) ? nullptr : resourcesSelected.top();
}

void RE_ResourceManager::PopSelected(bool all)
{
	if (resourcesSelected.empty())
		return;

	do {
		resourcesSelected.pop();
	} while (all && !resourcesSelected.empty());
}

ResourceContainer* RE_ResourceManager::At(const char* md5) const
{
	return resources.at(md5);
}

const char* RE_ResourceManager::ReferenceByMeta(const char* metaPath, Resource_Type type)
{
	const char* retMD5 = nullptr;
	ResourceContainer* newContainer = nullptr;
	switch (type)
	{
	case R_SHADER:
		newContainer = (ResourceContainer*)new RE_Shader(metaPath);
		break;
	case R_TEXTURE:
		newContainer = (ResourceContainer*)new RE_Texture(metaPath);
		break;
	case R_PREFAB:
		newContainer = (ResourceContainer*)new RE_Prefab(metaPath);
		break;
	case R_SKYBOX:
		newContainer = (ResourceContainer*)new RE_SkyBox(metaPath);
		break;
	case R_MATERIAL:
		newContainer = (ResourceContainer*)new RE_Material(metaPath);
		break;
	case R_MODEL:
		newContainer = (ResourceContainer*)new RE_Model(metaPath);
		break;
	case R_SCENE:
		newContainer = (ResourceContainer*)new RE_Scene(metaPath);
		break;
	}
	newContainer->LoadMeta();
	retMD5 = Reference(newContainer);
	return retMD5;
}

unsigned int RE_ResourceManager::TotalReferences() const
{
	return resources.size();
}

eastl::vector<const char*> RE_ResourceManager::GetAllResourcesActiveByType(Resource_Type resT)
{
	eastl::vector<ResourceContainer*> resourcesByType = GetResourcesByType(resT);
	eastl::vector<const char*> ret;
	while (!resourcesByType.empty()) {
		const char* resMD5 = resourcesByType.back()->GetMD5();
		if (resourcesCounter.at(resMD5) > 0) ret.push_back(resMD5);
		resourcesByType.pop_back();
	}
	return ret;
}

eastl::vector<ResourceContainer*> RE_ResourceManager::GetResourcesByType(Resource_Type type)
{
	eastl::vector<ResourceContainer*> ret;
	for (auto resource : resources)
	{
		if (resource.second->GetType() == type)
			ret.push_back(resource.second);
	}
	return ret;
}

const char* RE_ResourceManager::IsReference(const char* md5, Resource_Type type)
{
	const char* ret = nullptr;
	if (type == Resource_Type::R_UNDEFINED) {
		for (auto resource : resources)
		{
			if (eastl::Compare(resource.second->GetMD5(), md5, 32) == 0)
				ret = resource.first;
		}
	}
	else
	{
		eastl::vector<ResourceContainer*> tResources = GetResourcesByType(type);
		for (auto resource : tResources)
		{
			if (eastl::Compare(resource->GetMD5(), md5, 32) == 0)
				ret = resource->GetMD5();
		}
	}
	return ret;
}

const char * RE_ResourceManager::FindMD5ByMETAPath(const char * metaPath, Resource_Type type)
{
	const char* ret = nullptr;
	if (type == Resource_Type::R_UNDEFINED) {
		for (auto resource : resources)
		{
			if (eastl::Compare(resource.second->GetMetaPath(), metaPath, eastl::CharStrlen(resource.second->GetMetaPath())) == 0)
				ret = resource.first;
		}
	}
	else
	{
		eastl::vector<ResourceContainer*> tResources = GetResourcesByType(type);
		for (auto resource : tResources)
		{
			if (eastl::Compare(resource->GetMetaPath(), metaPath, eastl::CharStrlen(resource->GetMetaPath())) == 0)
				ret = resource->GetMD5();
		}
	}
	return ret;
}

const char* RE_ResourceManager::FindMD5ByLibraryPath(const char* libraryPath, Resource_Type type)
{
	const char* ret = nullptr;
	if (type == Resource_Type::R_UNDEFINED) {
		for (auto resource : resources)
		{
			if (eastl::Compare(resource.second->GetLibraryPath(), libraryPath, eastl::CharStrlen(resource.second->GetLibraryPath())) == 0)
				ret = resource.first;
		}
	}
	else
	{
		eastl::vector<ResourceContainer*> tResources = GetResourcesByType(type);
		for (auto resource : tResources)
		{
			if (eastl::Compare(resource->GetLibraryPath(), libraryPath, eastl::CharStrlen(resource->GetLibraryPath())) == 0)
				ret = resource->GetMD5();
		}
	}
	return ret;
}

const char * RE_ResourceManager::FindMD5ByAssetsPath(const char * assetsPath, Resource_Type type)
{
	const char* ret = nullptr;
	if (type == Resource_Type::R_UNDEFINED) {
		for (auto resource : resources)
		{
			if (eastl::Compare(resource.second->GetAssetPath(), assetsPath, eastl::CharStrlen(resource.second->GetAssetPath())) == 0)
				ret = resource.first;
		}
	}
	else
	{
		eastl::vector<ResourceContainer*> tResources = GetResourcesByType(type);
		for (auto resource : tResources)
		{
			if (eastl::Compare(resource->GetAssetPath(), assetsPath, eastl::CharStrlen(resource->GetAssetPath())) == 0)
				ret = resource->GetMD5();
		}
	}
	return ret;
}

const char* RE_ResourceManager::CheckOrFindMeshOnLibrary(const char* librariPath)
{
	const char* meshMD5 = nullptr;

	meshMD5 = FindMD5ByLibraryPath(librariPath, Resource_Type::R_MESH);

	if (!meshMD5) {
		if (App->fs->Exists(librariPath)) {
			RE_Mesh* newMesh = new RE_Mesh();
			newMesh->SetLibraryPath(librariPath);
			newMesh->SetType(Resource_Type::R_MESH);
			meshMD5 = Reference(newMesh);
		}
	}

	if (!meshMD5) LOG_ERROR("Error finding library mesh:\n%s", librariPath);

	return meshMD5;
}

void RE_ResourceManager::ThumbnailResources()
{
	for (auto res : resources) App->thumbnail->Add(res.first);
}

const char* RE_ResourceManager::ImportModel(const char* assetPath)
{
	Event::PauseEvents();
	eastl::string path(assetPath);
	eastl::string filename = path.substr(path.find_last_of("/") + 1);
	eastl::string name = filename.substr(0, filename.find_last_of("."));
	eastl::string extension = filename.substr(filename.find_last_of(".") + 1);

	RE_Model* newModel = new RE_Model();
	newModel->SetName(name.c_str());
	newModel->SetAssetPath(assetPath);
	newModel->SetType(Resource_Type::R_MODEL);
	newModel->Import(false);
	newModel->SaveMeta();
	Event::ResumeEvents();

	return Reference(newModel);
}
const char* RE_ResourceManager::ImportTexture(const char* assetPath)
{
	eastl::string path(assetPath);
	eastl::string filename = path.substr(path.find_last_of("/") + 1);
	eastl::string name = filename.substr(0, filename.find_last_of("."));
	eastl::string extension = filename.substr(filename.find_last_of(".") + 1);

	RE_Texture* newTexture = new RE_Texture();
	newTexture->SetName(name.c_str());
	newTexture->SetAssetPath(assetPath);
	newTexture->SetType(Resource_Type::R_TEXTURE);
	newTexture->Import(false);
	newTexture->SaveMeta();

	return Reference(newTexture);
}

const char* RE_ResourceManager::ImportMaterial(const char* assetPath)
{
	eastl::string path(assetPath);
	eastl::string filename = path.substr(path.find_last_of("/") + 1);
	eastl::string name = filename.substr(0, filename.find_last_of("."));
	eastl::string extension = filename.substr(filename.find_last_of(".") + 1);

	RE_Material* newMaterial = new RE_Material();
	newMaterial->SetName(name.c_str());
	newMaterial->SetAssetPath(assetPath);
	newMaterial->SetType(Resource_Type::R_MATERIAL);
	newMaterial->Import(false);
	newMaterial->SaveMeta();

	return Reference(newMaterial);
}

const char* RE_ResourceManager::ImportSkyBox(const char* assetPath)
{
	eastl::string path(assetPath);
	eastl::string filename = path.substr(path.find_last_of("/") + 1);
	eastl::string name = filename.substr(0, filename.find_last_of("."));
	eastl::string extension = filename.substr(filename.find_last_of(".") + 1);

	RE_SkyBox* newSkyBox = new RE_SkyBox();
	newSkyBox->SetName(name.c_str());
	newSkyBox->SetAssetPath(assetPath);
	newSkyBox->SetType(Resource_Type::R_SKYBOX);
	newSkyBox->Import(false);
	newSkyBox->SaveMeta();

	return Reference(newSkyBox);
}

const char* RE_ResourceManager::ImportPrefab(const char* assetPath)
{
	Event::PauseEvents();
	eastl::string path(assetPath);
	eastl::string filename = path.substr(path.find_last_of("/") + 1);
	eastl::string name = filename.substr(0, filename.find_last_of("."));
	eastl::string extension = filename.substr(filename.find_last_of(".") + 1);

	RE_Prefab* newPrefab = new RE_Prefab();
	newPrefab->SetName(name.c_str());
	newPrefab->SetAssetPath(assetPath);
	newPrefab->SetType(Resource_Type::R_PREFAB);
	newPrefab->Import(false);
	newPrefab->SaveMeta();
	Event::ResumeEvents();

	return Reference(newPrefab);
}

const char* RE_ResourceManager::ImportScene(const char* assetPath)
{
	Event::PauseEvents();
	eastl::string path(assetPath);
	eastl::string filename = path.substr(path.find_last_of("/") + 1);
	eastl::string name = filename.substr(0, filename.find_last_of("."));
	eastl::string extension = filename.substr(filename.find_last_of(".") + 1);

	RE_Scene* newScene = new RE_Scene();
	newScene->SetName(name.c_str());
	newScene->SetAssetPath(assetPath);
	newScene->SetType(Resource_Type::R_SCENE);
	newScene->Import(false);
	newScene->SaveMeta();
	Event::ResumeEvents();

	return Reference(newScene);
}

unsigned int RE_ResourceManager::TotalReferenceCount(const char* resMD5) const
{
	return resourcesCounter.at(resMD5);
}