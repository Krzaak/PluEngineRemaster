//
// Created by Plutex on 2026-01-24.
//

#include "PluEngine/Core/Events/EventDispatcher.h"

#include <algorithm>

namespace Plu
{
	EventHandle EventDispatcher::Subscribe(const String& eventName, EventCallback callback) {
		EventHandle handle = s_NextHandle++;
		m_Subscribers[eventName].PushBack({ handle, callback });
		return handle;
	}

	void EventDispatcher::Unsubscribe(const String& eventName, EventHandle handle) {
		if (m_Subscribers.Contains(eventName)) {
			auto subs = m_Subscribers.Find(eventName);
			subs->RemoveIf([handle](const Subscription& s) { return s.Handle == handle; });
		}
	}

	void EventDispatcher::Dispatch(const String& eventName, void* data) {
		if (m_Subscribers.Contains(eventName)) {
			auto subsCopy = m_Subscribers.Find(eventName);
			for (const auto& sub : *subsCopy) {
				sub.Callback(data);
			}
		}
	}
}
