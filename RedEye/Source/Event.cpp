#include "Event.h"
#include "EventListener.h"
#include "SDL2\include\SDL_timer.h"

#include <EASTL/queue.h>

bool Event::paused = false;
eastl::queue<Event> events_queue;

Event::Event(RE_EventType t, EventListener * lis, Cvar d1, Cvar d2) : type(t), listener(lis), data1(d1), data2(d2), timestamp(SDL_GetTicks()) {}
Event::Event(Event& e) : type(e.type), listener(e.listener), data1(e.data1), data2(e.data2), timestamp(SDL_GetTicks()) {}

void Event::Push(RE_EventType t, EventListener* lis, Cvar d1, Cvar d2)
{
	if (!Event::paused) events_queue.push(Event(t, lis, d1, d2));
}
void Event::PushForced(RE_EventType t, EventListener* lis, Cvar d1, Cvar d2)
{
	events_queue.push(Event(t, lis, d1, d2));
}

void Event::PumpAll()
{
	while (!events_queue.empty())
	{
		const Event e = events_queue.front();

		if (e.IsValid())
			e.CallListener();

		events_queue.pop();
	}
}

void Event::ClearQueue() { while (!events_queue.empty()) { events_queue.pop(); } }
void Event::ResumeEvents() { paused = false; }
void Event::PauseEvents() { paused = true; }
bool Event::isPaused() { return paused; }

void Event::CallListener() const { if (listener) listener->RecieveEvent(*this); }
bool Event::IsValid() const { return type < MAX_EVENT_TYPES&& listener != nullptr; }
