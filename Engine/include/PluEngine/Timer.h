//
// Created by Plutex on 2026-01-24.
//

#ifndef PLUENGINE_TIMER_H
#define PLUENGINE_TIMER_H

#include <chrono>
#include <string>
#include <unordered_map>
#include "Log.h"
#include "PluSTL_FWD.h"

namespace Plu {

    class Timer {
    public:
        Timer(const String& name)
            : m_Name(name), m_Stopped(false) {
            m_StartTimepoint = std::chrono::high_resolution_clock::now();
        }

        ~Timer() {
            if (!m_Stopped) Stop();
        }

        void Stop() {
            auto endTimepoint = std::chrono::high_resolution_clock::now();
            long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
            long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

            float duration = (end - start) * 0.001f; // konwersja na milisekundy

            // Używamy Twoich istniejących makr logowania
            if (duration < 0.2) {
                PLU_CORE_TRACE("Timer {0}: {1}ms", m_Name.CStr(), duration);
            } else if (duration < 0.8) {
                PLU_CORE_INFO("Timer {0}: {1}ms", m_Name.CStr(), duration);
            } else {
                PLU_CORE_WARN("Timer {0}: {1}ms", m_Name.CStr(), duration);
            }
            m_Stopped = true;
        }

    private:
        String m_Name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
        bool m_Stopped;
    };

    // Singleton do zarządzania ręcznymi timerami (dla PLU_TIMER_END)
    class TimerManager {
    public:
        static void StartTimer(const String& name) {
            GetTimers().Insert(name, CreateOwning<Timer>(name));
        }

        static void EndTimer(const String& name) {
            auto& timers = GetTimers();
            if (timers.Contains(name)) {
                timers.Remove(name); // Destruktor klasy Timer zajmie się logowaniem
            } else {
                PLU_CORE_WARN("Próba zatrzymania nieistniejącego timera: {0}", name.CStr());
            }
        }

    private:
        static GameHashMap<String, TOwningPointer<Timer>>& GetTimers() {
            static GameHashMap<String, TOwningPointer<Timer>> s_Timers;
            return s_Timers;
        }
    };
}

// --- Makra ---

// Automatyczny timer działający do końca bieżącego scope'a
#define PLU_PROFILE_SCOPE(name) ::Plu::Timer timer##__LINE__(name)

// Ręczne sterowanie timerami (w stylu JS)
#define PLU_TIMER_START(name) ::Plu::TimerManager::StartTimer(name)
#define PLU_TIMER_END(name)   ::Plu::TimerManager::EndTimer(name)

#endif //PLUENGINE_TIMER_H