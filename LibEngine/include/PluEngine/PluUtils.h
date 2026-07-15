//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_PLUUTILS_H
#define PLUENGINE_PLUUTILS_H

#include "Core.h"
#include "PluSTL_FWD.h"
#include "PluTypes.h"
#include "Jolt/Jolt.h"
#include <cmath>

#ifdef PLU_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace Plu
{
	PLU_API PathW GetEngineResourcesDir();

	inline PathW GetExePath()
	{
#if defined(PLU_PLATFORM_WINDOWS)

		wchar_t buffer[MAX_PATH];
		DWORD size = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

		if (size == 0) {
			throw std::runtime_error("GetModuleFileNameW failed");
		}

		return buffer;

#elif defined(PLU_PLATFORM_LINUX)

		char buffer[PATH_MAX];
		ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

		if (len == -1) {
			throw std::runtime_error("readlink(/proc/self/exe) failed");
		}

		buffer[len] = '\0';
		return PathW(StringW::FromNarrow(buffer));

#endif
	}


	PLU_API Path GetSystemUserPath();


	PLU_FUNCTION()
	PLU_API Vec3 GetForwardVector(Vec3 rot);

	PLU_FUNCTION()
	PLU_API Vec3 GetRightVector(Vec3 rot);

	PLU_FUNCTION()
	PLU_API Vec3 GetUpVector(Vec3 rot);

	PLU_FUNCTION()
	PLU_API double ClampD(double val, double min, double max);
	PLU_FUNCTION()
	PLU_API float ClampF(float val, float min, float max);
	PLU_FUNCTION()
	PLU_API int ClampI(int val, int min, int max);
	PLU_FUNCTION()
	PLU_API float ClampAngle(float angle, float min, float max);
	
	PLU_API Vec3 GetLookAtRotatorDegrees(const Vec3& eye, const Vec3& target);
	PLU_API Vec3 GetRotatedPointWithRadius(const Vec3& center, float radius, float angleDegrees, const Vec3& axis);
	PLU_API Vec3 GetSphericalOrbitPoint(const Vec3& center, float radius, float yawDegrees, float pitchDegrees);

	PLU_API void NormalizeVec3Rotation(Vec3* vec);

	// --- Decompose a transform matrix into its components ---------------------------------
	// Location is the translation column, scale is the length of each basis vector, and
	// rotation is returned as Euler angles in degrees (pitch=X, yaw=Y, roll=Z), matching the
	// rotation convention used everywhere else in the engine.
	PLU_API Vec3 GetLocationFromMatrix(const Matrix4& matrix);
	PLU_API Vec3 GetScaleFromMatrix(const Matrix4& matrix);
	PLU_API Vec3 GetRotationFromMatrix(const Matrix4& matrix);

	// --- Picking: UInt32 ID <-> color ------------------------------------------------------
	// Pack a 32-bit id (e.g. a truncated UUID / per-frame object index) into an RGBA color so it
	// can be written to a picking framebuffer and read back. Each byte of the id maps to one
	// channel, normalized to [0,1] for a regular RGBA8 target:
	//   R = bits  0..7, G = bits  8..15, B = bits 16..23, A = bits 24..31
	// Inverse is UnpackColorToUInt32; rounding makes the round-trip exact for 8-bit channels.
	inline Vec4 PackUInt32ToColor(UInt32 id)
	{
		return Vec4(
			static_cast<float>( id        & 0xFFu) / 255.0f,
			static_cast<float>((id >>  8u) & 0xFFu) / 255.0f,
			static_cast<float>((id >> 16u) & 0xFFu) / 255.0f,
			static_cast<float>((id >> 24u) & 0xFFu) / 255.0f);
	}

	inline UInt32 UnpackColorToUInt32(const Vec4& color)
	{
		const UInt32 r = static_cast<UInt32>(std::lround(color.r * 255.0f)) & 0xFFu;
		const UInt32 g = static_cast<UInt32>(std::lround(color.g * 255.0f)) & 0xFFu;
		const UInt32 b = static_cast<UInt32>(std::lround(color.b * 255.0f)) & 0xFFu;
		const UInt32 a = static_cast<UInt32>(std::lround(color.a * 255.0f)) & 0xFFu;
		return r | (g << 8u) | (b << 16u) | (a << 24u);
	}

	// --- Per-thread frame timing ----------------------------------------------------------
	// The Main thread (game/UI loop) and the Render thread run independently (decoupled via the
	// RenderSnapshot triple buffer), so they tick at different rates. Each loop publishes its
	// last frame delta through these setters; the getters return the corresponding FPS.
	// All four are thread-safe (backed by atomics).
	PLU_API void SetMainThreadDeltaTime(float deltaSeconds);
	PLU_API void SetRenderThreadDeltaTime(float deltaSeconds);

	PLU_FUNCTION()
	PLU_API float GetMainThreadFPS();
	PLU_FUNCTION()
	PLU_API float GetRenderThreadFPS();

	PLU_API String MakeStringForDisplay(String text);
	PLU_API String PrepareCodeForDistribution(String code);

	static JPH::RVec3 ToJPH(const Vec3& V) {
		return {V.x, V.y, V.z};
	}

	static Vec3 ToGLM(const JPH::RVec3& V) {
		return {V.GetX(), V.GetY(), V.GetZ()};
	}

	static JPH::Vec3 ToJPHVec3(const Vec3& V) {
		return {V.x, V.y, V.z};
	}

	static Vec3 ToGLMFromVec3(const JPH::Vec3& V) {
		return {V.GetX(), V.GetY(), V.GetZ()};
	}
}

#endif //PLUENGINE_PLUUTILS_H