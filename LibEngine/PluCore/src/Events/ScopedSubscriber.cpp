//
// Created by Plutex on 2026-01-24.
//

#include "PluEngine/Core/Events/ScopedSubscriber.h"

namespace Plu
{
	ScopedSubscriber::ScopedSubscriber(EventDispatcher* dispatcher, const String& eventName, EventCallback callback)
		: m_Dispatcher(dispatcher), m_EventName(eventName) {
		if (m_Dispatcher) {
			m_Handle = m_Dispatcher->Subscribe(m_EventName, callback);
		}
	}

	ScopedSubscriber::~ScopedSubscriber() {
		Disconnect();
	}

	void ScopedSubscriber::Disconnect() {
		if (m_Dispatcher && m_Handle != 0) {
			m_Dispatcher->Unsubscribe(m_EventName, m_Handle);
			m_Handle = 0;
			m_Dispatcher = nullptr;
		}
	}

	ScopedSubscriber::ScopedSubscriber(ScopedSubscriber&& other) noexcept {
		*this = std::move(other);
	}

	ScopedSubscriber& ScopedSubscriber::operator=(ScopedSubscriber&& other) noexcept {
		if (this != &other) {
			Disconnect();
			m_Dispatcher = other.m_Dispatcher;
			m_EventName = std::move(other.m_EventName);
			m_Handle = other.m_Handle;

			other.m_Dispatcher = nullptr;
			other.m_Handle = 0;
		}
		return *this;
	}
}