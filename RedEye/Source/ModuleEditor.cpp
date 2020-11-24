 #include "ModuleEditor.h"

#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleInput.h"
#include "ModuleScene.h"
#include "ModuleRenderer3D.h"
#include "EditorWindows.h"
#include "RE_TimeManager.h"
#include "OutputLog.h"
#include "RE_GOManager.h"
#include "RE_CameraManager.h"
#include "RE_PrimitiveManager.h"
#include "QuadTree.h"
#include "RE_GLCacheManager.h"
#include "RE_ThumbnailManager.h"
#include "RE_FileSystem.h"

#include "RE_ResourceManager.h"
#include "RE_Prefab.h"

#include "MathGeoLib\include\MathGeoLib.h"
#include "ImGui\imgui_impl_opengl3.h"
#include "ImGui\imgui_impl_sdl.h"
#include "ImGuizmo\ImGuizmo.h"
#include "ImGui/imgui_internal.h"
#include "glew\include\glew.h"
#include "SDL2\include\SDL.h"

#include <EASTL/stack.h>
#include <EASTL/queue.h>

ModuleEditor::ModuleEditor(const char* name, bool start_enabled) : Module(name, start_enabled)
{
	popupWindow = new PopUpWindow();
	windows.push_back(console = new ConsoleWindow());
	windows.push_back(config = new ConfigWindow());
	windows.push_back(heriarchy = new HeriarchyWindow());
	windows.push_back(properties = new PropertiesWindow());
	windows.push_back(play_pause = new PlayPauseWindow());
	about = new AboutWindow();

	tools.push_back(rng = new RandomTest());
	materialeditor = new MaterialEditorWindow();
	skyboxeditor = new SkyBoxEditorWindow();
	shadereditor = new ShaderEditorWindow();
	texteditormanager = new TextEditorManagerWindow();
	waterplaneResourceWindow = new WaterPlaneResourceWindow();

	sceneEditorWindow = new SceneEditorWindow();
	sceneGameWindow = new SceneGameWindow();

	tools.push_back(transDebInfo = new TransformDebugWindow());
	
	grid_size[0] = grid_size[1] = 1.f;

	all_aabb_color[0] = 0.f;
	all_aabb_color[1] = 1.f;
	all_aabb_color[2] = 0.f;

	sel_aabb_color[0] = 1.f;
	sel_aabb_color[1] = .5f;
	sel_aabb_color[2] = 0.f;

	quad_tree_color[0] = 1.f;
	quad_tree_color[1] = 1.f;
	quad_tree_color[2] = 0.f;

	frustum_color[0] = 0.f;
	frustum_color[1] = 1.f;
	frustum_color[2] = 1.f;
}

ModuleEditor::~ModuleEditor()
{
	windows.clear();
	tools.clear();
	editorCommands.Clear();
}

bool ModuleEditor::Init(JSONNode* node)
{
	bool ret = true;

	// ImGui
	RE_LOG_SECONDARY("Init ImGui");
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	if (ret = ImGui_ImplSDL2_InitForOpenGL(App::window->GetWindow(), App::renderer3d->GetWindowContext()))
	{
		if (ret = ImGui_ImplOpenGL3_Init())
			App::ReportSoftware("ImGui", IMGUI_VERSION, "https://github.com/ocornut/imgui");
		else
			RE_LOG_ERROR("ImGui could not OpenGL3_Init!");
	}
	else
		RE_LOG_ERROR("ImGui could not SDL2_InitForOpenGL!");

	return ret;
}

bool ModuleEditor::Start()
{
	windows.push_back(assets = new AssetsWindow());
	windows.push_back(wwise = new WwiseWindow());

	grid = new RE_CompGrid();
	grid->GridSetUp();

	// FOCUS CAMERA
	UID first = ModuleScene::GetRootCPtr()->GetFirstChildUID();
	if (first) SetSelected(first);

	return true;
}

update_status ModuleEditor::PreUpdate()
{
	OPTICK_CATEGORY("PreUpdate ModuleEditor", Optick::Category::GameLogic);
	ImGuizmo::SetOrthographic(!RE_CameraManager::EditorCamera()->isPrespective());
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(App::window->GetWindow());
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	return UPDATE_CONTINUE;
}

update_status ModuleEditor::Update()
{
	OPTICK_CATEGORY("Update ModuleEditor", Optick::Category::UI);
	if (show_all)
	{
		// Main Menu Bar
		if (ImGui::BeginMainMenuBar())
		{
			// File
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem(" New Scene"))
				{
					if (App::scene->HasChanges()) App::editor->popupWindow->PopUpSave(false, true);
					else App::scene->NewEmptyScene();
				}
				if (ImGui::MenuItem(" Save Scene") && App::scene->HasChanges())
				{
					if (App::scene->isNewScene()) App::editor->popupWindow->PopUpSave();
					else App::scene->SaveScene();
				}
				if (ImGui::MenuItem(" Exit", "	Esc"))
				{
					if (App::scene->HasChanges()) App::editor->popupWindow->PopUpSave(true);
					else Event::Push(REQUEST_QUIT, App::Ptr());
				}
				ImGui::EndMenu();
			}

			//Edit
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "LCTRL + Z", false, editorCommands.canUndo()))
					editorCommands.undo();

				if (ImGui::MenuItem("Redo", "LCTRL + Y", false, editorCommands.canRedo()))
					editorCommands.redo();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Assets"))
			{
				if (ImGui::BeginMenu("Create")) {

					if (ImGui::MenuItem("Material", materialeditor->IsActive() ? "Hide" : "Open"))
						materialeditor->SwitchActive();

					if (ImGui::MenuItem("Shader", shadereditor->IsActive() ? "Hide" : "Open"))
						shadereditor->SwitchActive();

					if (ImGui::MenuItem("Skybox", skyboxeditor->IsActive() ? "Hide" : "Open"))
						skyboxeditor->SwitchActive();

					if (ImGui::MenuItem("Water Resources", waterplaneResourceWindow->IsActive() ? "Hide" : "Open"))
						waterplaneResourceWindow->SwitchActive();

					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Gameobject"))
			{
				DrawGameObjectItems();
				ImGui::EndMenu();
			}

			// View
			if (ImGui::BeginMenu("View"))
			{
				for (auto window : windows)
					if (ImGui::MenuItem(window->Name(), window->IsActive() ? "Hide" : "Open"))
						window->SwitchActive();

				// Tools submenu
				if (ImGui::BeginMenu("Tools"))
				{
					for (auto tool : tools)
						if (ImGui::MenuItem(tool->Name(), tool->IsActive() ? "Hide" : "Open"))
							tool->SwitchActive();

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			// Help
			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem(show_demo ? "Close Gui Demo" : "Open Gui Demo"))
					show_demo = !show_demo;
				if (ImGui::MenuItem("Documentation"))
					BROWSER("https://github.com/juliamauri/RedEye-Engine/wiki");
				if (ImGui::MenuItem("Download Latest"))
					BROWSER("https://github.com/juliamauri/RedEye-Engine/releases");
				if (ImGui::MenuItem("Report a Bug"))
					BROWSER("https://github.com/juliamauri/RedEye-Engine/issues");
				if (ImGui::MenuItem("About", about->IsActive() ? "Hide" : "Open"))
					about->SwitchActive();

				ImGui::EndMenu();
			}

			if (show_demo) ImGui::ShowTestWindow();

			ImGui::EndMainMenuBar();
		}

		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None; // | ImGuiDockNodeFlags_PassthruCentralNode;
		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		
		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", (bool*)true, window_flags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (popupWindow->IsActive()) popupWindow->DrawWindow();

		// Draw Windows
		for (auto window : windows)
			if (window->IsActive())
				window->DrawWindow(popUpFocus);

		if (about && about->IsActive())
			about->DrawWindow(popUpFocus); // (not in windows' list)

		for (auto tool : tools)
			if (tool->IsActive())
				tool->DrawWindow(popUpFocus);

		if (materialeditor->IsActive()) materialeditor->DrawWindow(popUpFocus);
		if (shadereditor->IsActive()) shadereditor->DrawWindow(popUpFocus);
		if (skyboxeditor->IsActive()) skyboxeditor->DrawWindow(popUpFocus);
		if (waterplaneResourceWindow->IsActive()) waterplaneResourceWindow->DrawWindow(popUpFocus);

		texteditormanager->DrawWindow(popUpFocus);
	}
	
	// Toggle show editor on F1
	if(App::input->CheckKey(SDL_SCANCODE_F1))
		show_all = !show_all;

	// CAMERA CONTROLS
	UpdateCamera();

	if (App::input->GetKey(SDL_SCANCODE_LCTRL) == KEY_STATE::KEY_REPEAT)
	{
		if (App::input->GetKey(SDL_SCANCODE_Z) == KEY_STATE::KEY_DOWN)
			editorCommands.undo();

		if (App::input->GetKey(SDL_SCANCODE_Y) == KEY_STATE::KEY_DOWN)
			editorCommands.redo();
	}

	ImGui::End();

	return UPDATE_CONTINUE;
}

bool ModuleEditor::CleanUp()
{
	while (!windows.empty())
	{
		delete *(windows.rbegin());
		windows.pop_back();
	}

	while (!tools.empty())
	{
		delete *(tools.rbegin());
		tools.pop_back();
	}

	DEL(materialeditor);
	DEL(skyboxeditor);
	DEL(shadereditor);

	DEL(popupWindow);
	DEL(about);
	DEL(texteditormanager);
	DEL(waterplaneResourceWindow);

	DEL(sceneEditorWindow);
	DEL(sceneGameWindow);

	DEL(grid);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	return true;
}

void ModuleEditor::RecieveEvent(const Event& e)
{
	switch (e.type)
	{
	case EDITORWINDOWCHANGED: sceneEditorWindow->UpdateViewPort(); break;
	case GAMEWINDOWCHANGED: sceneGameWindow->UpdateViewPort(); break;
	case UPDATE_SCENE_WINDOWS:
	{
		if(e.data1.AsGO()) sceneGameWindow->Recalc();
		else sceneEditorWindow->Recalc();
		break;
	}
	case EDITOR_SCENE_RAYCAST:
	{
		// Mouse Pick
		RE_CompCamera* camera = RE_CameraManager::EditorCamera();
		float width, height;
		camera->GetTargetWidthHeight(width, height);

		OPTICK_CATEGORY("Update ModuleEditor Camera RayCast", Optick::Category::Camera);
		UID hit = App::scene->RayCastSelect(
			math::Ray(camera->GetFrustum().UnProjectLineSegment(
			(e.data1.AsFloat() -(width / 2.0f)) / (width / 2.0f),
				((height - e.data2.AsFloat()) - (height / 2.0f)) / (height / 2.0f))));

		if (hit) SetSelected(hit);

		break;
	}
	}
}

void ModuleEditor::Draw() const
{
	OPTICK_CATEGORY("ImGui Rend", Optick::Category::Rendering);

	sceneEditorWindow->DrawWindow();
	sceneGameWindow->DrawWindow();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		//GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		//glfwMakeContextCurrent(backup_current_context);
	}
}

void ModuleEditor::DrawEditor()
{
	if (ImGui::CollapsingHeader(GetName()))
	{
		ImGui::DragFloat("Camera speed", &cam_speed, 0.1f, 0.1f, 100.0f, "%.1f");
		ImGui::DragFloat("Camera sensitivity", &cam_sensitivity, 0.01f, 0.01f, 1.0f, "%.2f");

		RE_CameraManager::EditorCamera()->DrawAsEditorProperties();

		ImGui::Separator();
		ImGui::Checkbox("Select on mouse click", &select_on_mc);
		ImGui::Checkbox("Focus on Select", &focus_on_select);
		ImGui::Separator();

		// Debug Drawing
		ImGui::Checkbox("Debug Draw", &debug_drawing);
		if (debug_drawing)
		{
			bool active_grid = grid->IsActive();
			if (ImGui::Checkbox("Draw Grid", &active_grid))
				grid->SetActive(active_grid);

			if (active_grid && ImGui::DragFloat2("Grid Size", grid_size, 0.2f, 0.01f, 100.0f, "%.1f"))
			{
				grid->GetTransformPtr()->SetScale(math::vec(grid_size[0], 0.f, grid_size[1]));
				grid->GetTransformPtr()->Update();
			}

			int aabb_d = aabb_drawing;
			if (ImGui::Combo("Draw AABB", &aabb_d, "None\0Selected only\0All\0All w/ different selected\0"))
				aabb_drawing = AABBDebugDrawing(aabb_d);

			if (aabb_drawing > SELECTED_ONLY) ImGui::ColorEdit3("Color AABB", all_aabb_color);
			if (aabb_drawing % 2 == 1) ImGui::ColorEdit3("Color Selected", sel_aabb_color);

			ImGui::Checkbox("Draw QuadTree", &draw_quad_tree);
			if (draw_quad_tree) ImGui::ColorEdit3("Color Quadtree", quad_tree_color);

			ImGui::Checkbox("Draw Camera Fustrums", &draw_cameras);
			if (draw_cameras) ImGui::ColorEdit3("Color Fustrum", frustum_color);
		}
	}
}

void ModuleEditor::DrawDebug(RE_CompCamera* current_camera) const
{
	OPTICK_CATEGORY("Debug Draw", Optick::Category::Debug);
	AABBDebugDrawing adapted_AABBdraw = (selected ? aabb_drawing : AABBDebugDrawing(aabb_drawing - 1));

	if (debug_drawing && ((adapted_AABBdraw != AABBDebugDrawing::NONE) || draw_quad_tree || draw_cameras))
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(current_camera->GetProjectionPtr());
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((current_camera->GetView()).ptr());
		glBegin(GL_LINES);

		// Draw Bounding Boxes
		switch (adapted_AABBdraw)
		{
		case SELECTED_ONLY:
		{
			glColor4f(sel_aabb_color[0], sel_aabb_color[1], sel_aabb_color[2], 1.0f);
			ModuleScene::GetGOCPtr(selected)->DrawGlobalAABB();
			break;
		}
		case ALL:
		{
			eastl::queue<const RE_GameObject*> objects;
			for (auto child : ModuleScene::GetRootCPtr()->GetChildsPtr())
				objects.push(child);

			if (!objects.empty())
			{
				glColor4f(all_aabb_color[0], all_aabb_color[1] * 255.0f, all_aabb_color[2], 1.0f);

				const RE_GameObject* object = nullptr;
				while (!objects.empty())
				{
					(object = objects.front())->DrawGlobalAABB();
					objects.pop();

					if (object->ChildCount() > 0u)
						for (auto child : object->GetChildsPtr())
							objects.push(child);
				}
			}

			break;
		}
		case ALL_AND_SELECTED:
		{
			glColor4f(sel_aabb_color[0], sel_aabb_color[1], sel_aabb_color[2], 1.0f);
			ModuleScene::GetGOCPtr(selected)->DrawGlobalAABB();

			eastl::queue<const RE_GameObject*> objects;
			for (auto child : ModuleScene::GetRootCPtr()->GetChildsPtr())
				objects.push(child);

			if (!objects.empty())
			{
				glColor4f(all_aabb_color[0], all_aabb_color[1], all_aabb_color[2], 1.0f);

				while (!objects.empty())
				{
					const RE_GameObject* object = objects.front();
					objects.pop();

					if (object->GetUID() != selected) object->DrawGlobalAABB();

					if (object->ChildCount() > 0)
						for (auto child : object->GetChildsPtr())
							objects.push(child);
				}
			}

			break;
		}
		}

		if (draw_quad_tree)
		{
			glColor4f(quad_tree_color[0], quad_tree_color[1], quad_tree_color[2], 1.0f);
			App::scene->DebugDraw();
		}

		if (draw_cameras)
		{
			glColor4f(frustum_color[0], frustum_color[1], frustum_color[2], 1.0f);
			for (auto cam : ModuleScene::GetScenePool()->GetAllCompPtr(C_CAMERA))
				static_cast<RE_CompCamera*>(cam)->DrawFrustum();
		}

		glEnd();

		//if (grid->IsActive()) grid->Draw();
	}
}

void ModuleEditor::DrawHeriarchy()
{
	UID to_select = 0ull, goToDelete_uid = 0ull;
	const RE_GameObject* root = ModuleScene::GetRootCPtr();
	UID root_uid = root->GetUID();
	if (root->ChildCount() > 0)
	{
		eastl::stack<RE_GameObject*> gos;
		for (auto child : root->GetChildsPtrReversed()) gos.push(child);

		unsigned int count = 0;
		while (!gos.empty())
		{
			RE_GameObject* go = gos.top();
			gos.pop();

			UID go_uid = go->GetUID();
			bool is_leaf = (go->ChildCount() == 0);

			ImGui::PushID(eastl::string("#HierarchyGOID" + eastl::to_string(count++)).c_str());
			if (ImGui::TreeNodeEx(go->name.c_str(), ImGuiTreeNodeFlags_(selected == go_uid ?
				(is_leaf ? ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_Leaf :
					ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick) :
				(is_leaf ? ImGuiTreeNodeFlags_Leaf :
					ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick))))
			{
				if (is_leaf) ImGui::TreePop();
				else for (auto child : go->GetChildsPtrReversed()) gos.push(child);
			}
			ImGui::PopID();

			if (ImGui::IsItemClicked()) to_select = go_uid;

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Create Prefab")) popupWindow->PopUpPrefab(go);
				ImGui::Separator();
				if (ImGui::MenuItem("Destroy GameObject")) goToDelete_uid = go_uid;
				ImGui::Separator();
				DrawGameObjectItems(go_uid);
				ImGui::EndPopup();
			}

			if (go->IsLastChild() && go->GetParentUID() != root_uid) ImGui::TreePop();
		}
	}

	if (to_select) SetSelected(to_select);
	if (goToDelete_uid != 0ull) {
		Event::Push(DESTROY_GO, App::scene, goToDelete_uid);
		if (selected == goToDelete_uid || ModuleScene::GetGOPtr(goToDelete_uid)->isParent(selected)) selected = 0ull;
	}
}

UID ModuleEditor::GetSelected() const { return selected; }

void ModuleEditor::SetSelected(const UID go, bool force_focus)
{
	selected = go;
	App::resources->PopSelected(true);
	if (force_focus || (focus_on_select && selected))
	{
		math::AABB box = ModuleScene::GetGOCPtr(selected)->GetGlobalBoundingBoxWithChilds();
		RE_CameraManager::EditorCamera()->Focus(box.CenterPoint(), box.HalfSize().Length());
	}
}

void ModuleEditor::DuplicateSelectedObject()
{
	if (selected)
	{
		const RE_GameObject* sel_go = ModuleScene::GetGOCPtr(selected);
		ModuleScene::GetGOPtr(ModuleScene::GetScenePool()->CopyGOandChilds(sel_go, sel_go->GetParentUID(), true))->UseResources();
	}
}

void ModuleEditor::LogToEditorConsole()
{
	if (console && !windows.empty()
		&& (console->file_filter < 0 || App::log->logHistory.back().caller_id == console->file_filter)
		&& console->categories[(int)App::log->logHistory.back().category])
	{
		console->console_buffer.append(App::log->logHistory.back().data.c_str());
		console->scroll_to_bot = true;
	}
}

bool ModuleEditor::AddSoftwareUsed(const char * name, const char * version, const char * website)
{
	bool ret;
	if (ret = (about != nullptr)) about->sw_info.push_back(SoftwareInfo(name, version, website));
	return ret;
}


void ModuleEditor::HandleSDLEvent(SDL_Event* e) { ImGui_ImplSDL2_ProcessEvent(e); }
void ModuleEditor::PopUpFocus(bool focus) { popUpFocus = focus; }
const char* ModuleEditor::GetAssetsPanelPath() const { return assets->GetCurrentDirPath(); }
void ModuleEditor::SelectUndefinedFile(eastl::string* toSelect) const { assets->SelectUndefined(toSelect); }

void ModuleEditor::OpenTextEditor(const char* filePath, eastl::string* filePathStr, const char* shadertTemplate, bool* open)
{
	texteditormanager->PushEditor(filePath, filePathStr, shadertTemplate, open);
}

void ModuleEditor::GetSceneWindowSize(unsigned int* widht, unsigned int* height)
{
	*widht = sceneEditorWindow->GetSceneWidht();
	*height = sceneEditorWindow->GetSceneHeight();
}

void ModuleEditor::CreatePrefab(const UID go, const char* name, bool identityRoot)
{
	RE_Prefab* newPrefab = new RE_Prefab();
	newPrefab->SetName(name);
	newPrefab->SetType(Resource_Type::R_PREFAB);
	Event::PauseEvents();

	newPrefab->Save(App::scene->GetScenePool()->GetNewPoolFromID(go), identityRoot, true);
	Event::ResumeEvents();
	newPrefab->SaveMeta();
	App::renderer3d->PushThumnailRend(App::resources->Reference(newPrefab));
}

void ModuleEditor::PushCommand(RE_Command* cmd) { editorCommands.PushCommand(cmd); }
void ModuleEditor::ClearCommands() { editorCommands.Clear(); }

void ModuleEditor::UpdateCamera()
{
	OPTICK_CATEGORY("Update ModuleEditor Camera", Optick::Category::Camera);
	RE_CompCamera* camera = RE_CameraManager::EditorCamera();
	if (sceneEditorWindow->isSelected())
	{
		const MouseData mouse = App::input->GetMouse();

		if (App::input->GetKey(SDL_SCANCODE_LALT) == KEY_REPEAT && mouse.GetButton(1) == KEY_REPEAT)
		{
			// Orbit
			if (selected && (mouse.mouse_x_motion || mouse.mouse_y_motion))
				camera->Orbit(cam_sensitivity * -mouse.mouse_x_motion, cam_sensitivity * mouse.mouse_y_motion, ModuleScene::GetGOCPtr(selected)->GetGlobalBoundingBoxWithChilds().CenterPoint());
		}
		else if ((App::input->GetKey(SDL_SCANCODE_F) == KEY_DOWN) && selected)
		{
			// Focus
			math::AABB box = ModuleScene::GetGOCPtr(selected)->GetGlobalBoundingBoxWithChilds();
			camera->Focus(box.CenterPoint(), box.HalfSize().Length());
		}
		else
		{
			if (mouse.GetButton(3) == KEY_REPEAT)
			{
				// Camera Speed
				float cameraSpeed = cam_speed * RE_TimeManager::GetDeltaTime();
				if (App::input->CheckKey(SDL_SCANCODE_LSHIFT, KEY_REPEAT)) cameraSpeed *= 2.0f;

				// Move
				if (App::input->CheckKey(SDL_SCANCODE_W, KEY_REPEAT))	  camera->LocalMove(Dir::FORWARD, cameraSpeed);
				if (App::input->CheckKey(SDL_SCANCODE_S, KEY_REPEAT))	  camera->LocalMove(Dir::BACKWARD, cameraSpeed);
				if (App::input->CheckKey(SDL_SCANCODE_A, KEY_REPEAT))	  camera->LocalMove(Dir::LEFT, cameraSpeed);
				if (App::input->CheckKey(SDL_SCANCODE_D, KEY_REPEAT))	  camera->LocalMove(Dir::RIGHT, cameraSpeed);
				if (App::input->CheckKey(SDL_SCANCODE_SPACE, KEY_REPEAT)) camera->LocalMove(Dir::UP, cameraSpeed);
				if (App::input->CheckKey(SDL_SCANCODE_C, KEY_REPEAT))	  camera->LocalMove(Dir::DOWN, cameraSpeed);

				// Rotate
				if (mouse.mouse_x_motion != 0 || mouse.mouse_y_motion != 0)
					camera->LocalPan(cam_sensitivity * -mouse.mouse_x_motion, cam_sensitivity * mouse.mouse_y_motion);
			}

			// Zoom
			if (mouse.mouse_wheel_motion != 0) camera->SetFOV(camera->GetVFOVDegrees() - mouse.mouse_wheel_motion);
		}
	}

	camera->Update();
}

void ModuleEditor::DrawGameObjectItems(const UID parent)
{
	if (ImGui::BeginMenu("Primitive"))
	{
		if (ImGui::MenuItem("Grid")) App::scene->CreatePrimitive(C_GRID, parent);
		if (ImGui::MenuItem("Cube")) App::scene->CreatePrimitive(C_CUBE, parent);
		if (ImGui::MenuItem("Dodecahedron")) App::scene->CreatePrimitive(C_DODECAHEDRON, parent);
		if (ImGui::MenuItem("Tetrahedron")) App::scene->CreatePrimitive(C_TETRAHEDRON, parent);
		if (ImGui::MenuItem("Octohedron")) App::scene->CreatePrimitive(C_OCTOHEDRON, parent);
		if (ImGui::MenuItem("Icosahedron")) App::scene->CreatePrimitive(C_ICOSAHEDRON, parent);
		if (ImGui::MenuItem("Plane")) App::scene->CreatePrimitive(C_PLANE, parent);
		// if (ImGui::MenuItem("Frustum")) App::scene->CreatePrimitive(C_FUSTRUM, parent);
		if (ImGui::MenuItem("Sphere")) App::scene->CreatePrimitive(C_SPHERE, parent);
		if (ImGui::MenuItem("Cylinder")) App::scene->CreatePrimitive(C_CYLINDER, parent);
		if (ImGui::MenuItem("HemiSphere")) App::scene->CreatePrimitive(C_HEMISHPERE, parent);
		if (ImGui::MenuItem("Torus")) App::scene->CreatePrimitive(C_TORUS, parent);
		if (ImGui::MenuItem("Trefoil Knot")) App::scene->CreatePrimitive(C_TREFOILKNOT, parent);
		if (ImGui::MenuItem("Rock")) App::scene->CreatePrimitive(C_ROCK, parent);

		ImGui::EndMenu();
	}

	if (ImGui::MenuItem("Camera")) App::scene->CreateCamera(parent);
	if (ImGui::MenuItem("Light")) App::scene->CreateLight(parent);
	if (ImGui::MenuItem("Max Lights")) App::scene->CreateMaxLights(parent);
	if (ImGui::MenuItem("Water")) App::scene->CreateWater(parent);
}
