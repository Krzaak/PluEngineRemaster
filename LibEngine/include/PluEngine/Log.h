//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_LOG_H
#define PLUENGINE_LOG_H

#include "Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Plu {

	class PLU_API Log {
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

#define PLU_CORE_TRACE(...)    ::Plu::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PLU_CORE_INFO(...)     ::Plu::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PLU_CORE_WARN(...)     ::Plu::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PLU_CORE_ERROR(...)    ::Plu::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PLU_CORE_CRITICAL(...) ::Plu::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define PLU_TRACE(...)         ::Plu::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PLU_INFO(...)          ::Plu::Log::GetClientLogger()->info(__VA_ARGS__)
#define PLU_WARN(...)          ::Plu::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PLU_ERROR(...)         ::Plu::Log::GetClientLogger()->error(__VA_ARGS__)
#define PLU_CRITICAL(...)      ::Plu::Log::GetClientLogger()->critical(__VA_ARGS__)

#endif //PLUENGINE_LOG_H