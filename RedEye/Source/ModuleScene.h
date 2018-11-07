#ifndef __MODULESCENE_H__
#define __MODULESCENE_H__

#include "Module.h"
#include "Event.h"
#include "QuadTree.h"

class RE_GameObject;

class ModuleScene : public Module
{
public:
	ModuleScene(const char* name, bool start_enabled = true);
	~ModuleScene();

	bool Start() override;
	update_status Update() override;
	bool CleanUp() override;

	void FileDrop(const char* file);
	void RecieveEvent(const Event* e) override;

	RE_GameObject* AddGO(const char* name = nullptr, RE_GameObject* parent = nullptr);

	void DrawScene();
	void DrawHeriarchy();
	void DrawFocusedProperties();

	void SetSelected(RE_GameObject* selected);
	RE_GameObject* GetSelected() const;

private:

	RE_GameObject* root = nullptr;
	RE_GameObject* selected = nullptr;

	bool draw_quad_tree = true;
	QTree quad_tree;

	//shaders
	unsigned int modelloading;
};


#endif // !__MODULESCENE_H__