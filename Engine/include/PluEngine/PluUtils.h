//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_PLUUTILS_H
#define PLUENGINE_PLUUTILS_H

#include "Core.h"
#include "PluSTL_FWD.h"
#include "PluTypes.h"
#include "Jolt/Jolt.h"

#ifdef PLU_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace Plu
{
	PathW GetEngineResourcesDir();

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
	
	Vec3 GetLookAtRotatorDegrees(const Vec3& eye, const Vec3& target);
	Vec3 GetRotatedPointWithRadius(const Vec3& center, float radius, float angleDegrees, const Vec3& axis);
	Vec3 GetSphericalOrbitPoint(const Vec3& center, float radius, float yawDegrees, float pitchDegrees);

	static JPH::RVec3 ToJPH(const Vec3& V) {
		return {V.x, V.y, V.z};
	}

	static Vec3 ToGLM(const JPH::RVec3& V) {
		return {V.GetX(), V.GetY(), V.GetZ()};
	}
}

#endif //PLUENGINE_PLUUTILS_H