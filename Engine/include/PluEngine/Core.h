//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_CORE_H
#define PLUENGINE_CORE_H

#include <cstdint>

using UInt64 = std::uint64_t;
using UInt32 = std::uint32_t;
using UInt16 = std::uint16_t;
using UInt8 = std::uint8_t;

using Int8 = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;


#ifdef _WIN32
	#ifdef _WIN64
		#define PLU_PLATFORM_WINDOWS
	#else
		#error "32-bit OSes are not supported!"
	#endif
#elif defined(__linux__)
	#define PLU_PLATFORM_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	#if TARGET_OS_IPHONE
		#define PLU_PLATFORM_IOS
	#elif TARGET_OS_MAC
		#define PLU_PLATFORM_MACOS
	#else
		#error "Unknown Apple Platform!"
	#endif
#else
	#error "Unknown Platform!"
#endif

#ifdef PLU_PLATFORM_WINDOWS
	#ifdef PLU_BUILD_DLL
		#define PLU_API __declspec(dllexport)
	#else
		#define PLU_API __declspec(dllimport)
	#endif
#else
	#define PLU_API __attribute__((visibility("default")))
#endif

#ifdef PLU_DEBUG
	#if defined(PLU_PLATFORM_WINDOWS)
		#define PLU_DEBUGBREAK() __debugbreak()
	#elif defined(PLU_PLATFORM_LINUX)
		#include <csignal>
		#define PLU_DEBUGBREAK() raise(SIGTRAP)
	#endif
	#define PLU_ENABLE_ASSERTS
#else
	#define PLU_DEBUGBREAK()
#endif

#ifdef PLU_ENABLE_ASSERTS
	#define PLU_ASSERT(x, ...) { if(!(x)) { PLU_ERROR("Assertion Failed: {0}", __VA_ARGS__); PLU_DEBUGBREAK(); } }
	#define PLU_CORE_ASSERT(x, ...) { if(!(x)) { PLU_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); PLU_DEBUGBREAK(); } }
#else
	#define PLU_ASSERT(x, ...)
	#define PLU_CORE_ASSERT(x, ...)
#endif

#define PLU_CLASS(...)
#define PLU_INTERFACE(...)
#define PLU_STRUCT(...)

//PLU_PROPERTY macro for in-engine reflection
//Args:
//	UuidFor="" - Requires type as PluUUID, specifies for what class is the uuid for in-editor UI
#define PLU_PROPERTY(...)

#endif //PLUENGINE_CORE_H