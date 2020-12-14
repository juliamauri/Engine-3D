#ifndef __MODULEINPUT_H__
#define __MODULEINPUT_H__

#include "Module.h"
#include <EASTL/queue.h>

#define MAX_MOUSE_BUTTONS 5

enum KEY_STATE : char
{
	KEY_IDLE = 0,
	KEY_DOWN,
	KEY_REPEAT,
	KEY_UP
};

struct MouseData
{
	KEY_STATE mouse_buttons[MAX_MOUSE_BUTTONS];

	int mouse_x = -1;
	int mouse_y = -1;
	int mouse_x_motion = 0;
	int mouse_y_motion = 0;
	int mouse_wheel_motion = 0;

	void ResetMotion();
	void UpdateButtons();

	KEY_STATE GetButton(int id) const;
	bool Moved() const;
};

class ModuleInput : public Module
{
public:
	ModuleInput();
	~ModuleInput();

	bool Init() override;
	void PreUpdate() override;
	void DrawEditor() override;
	void CleanUp() override;

	// Keyboard
	KEY_STATE GetKey(const unsigned int id) const;
	bool CheckKey(const unsigned int id, const KEY_STATE state = KEY_UP) const;

	// Mouse
	const MouseData& GetMouse() const;
	void SetMouseAtCenter();

	// Events
	void Push(RE_EventType t, EventListener* lis, RE_Cvar d1 = RE_Cvar(), RE_Cvar d2 = RE_Cvar());
	void PushForced(RE_EventType t, EventListener* lis, RE_Cvar d1 = RE_Cvar(), RE_Cvar d2 = RE_Cvar());
	void ResumeEvents() { events_paused = false; }
	void PauseEvents() { events_paused = true; }
	bool Paused() const { return events_paused; }

private:

	void HandleSDLEventQueue();

private:

	// Input devices
	KEY_STATE* keyboard = nullptr;
	MouseData mouse;

	// Events
	friend Event;
	bool events_paused = false;
	eastl::queue<Event> events_queue;
};

#endif // !__MODULEINPUT_H__