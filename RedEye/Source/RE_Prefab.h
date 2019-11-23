#ifndef __RE_PREFAB_H__
#define __RE_PREFAB_H__

class RE_GameObject;
#include "Resource.h"

// .refab
class RE_Prefab : public ResourceContainer
{
public:
	RE_Prefab();
	RE_Prefab(const char* metaPath);
	~RE_Prefab();

public:
	void LoadInMemory() override;
	void UnloadMemory() override;

	void Save(RE_GameObject* go);

	//Override from container, when setting name sets the assets path to Assets/Prefabs/name.refab
	//If you want in another path, set directly all path on SetAsetsPath
	void SetName(const char* name) override;

	//returns a new, needed destroy after use.
	RE_GameObject* GetRoot();

private:
	void AssetSave();
	void AssetLoad();
	void LibraryLoad();
	void LibrarySave();

private:
	RE_GameObject* loaded = nullptr;
	RE_GameObject* toSave = nullptr;
};

#endif // !__RE_PREFAB_H__