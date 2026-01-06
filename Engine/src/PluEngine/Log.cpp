//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Plu {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::Init() {
		// Format: [Czas] NazwaLoggera: Wiadomość
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = spdlog::stdout_color_mt("ENGINE");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}
}
