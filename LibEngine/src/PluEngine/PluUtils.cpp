//
// Created by Plutex on 2026-02-28.
//

#include "PluEngine/PluUtils.h"

#include <atomic>
#include <regex>
#include <sstream>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace
{
	// Last published frame delta (seconds) per thread. Written by the owning loop, read from
	// anywhere (e.g. editor panels). Relaxed atomics are enough — these are diagnostics, no
	// ordering dependency on other state.
	std::atomic<float> gMainThreadDeltaTime{0.0f};
	std::atomic<float> gRenderThreadDeltaTime{0.0f};

	// Same rationale as above: published by the render thread at the end of each
	// Renderer::RenderSnapshot, read from editor panels on the main thread.
	std::atomic<UInt32> gStatDrawCalls{0};
	std::atomic<UInt32> gStatInstancesDrawn{0};
	std::atomic<UInt32> gStatCulledCount{0};
}

void Plu::SetMainThreadDeltaTime(float deltaSeconds)
{
	gMainThreadDeltaTime.store(deltaSeconds, std::memory_order_relaxed);
}

void Plu::SetRenderThreadDeltaTime(float deltaSeconds)
{
	gRenderThreadDeltaTime.store(deltaSeconds, std::memory_order_relaxed);
}

float Plu::GetMainThreadFPS()
{
	const float delta = gMainThreadDeltaTime.load(std::memory_order_relaxed);
	return delta > 0.0f ? 1.0f / delta : 0.0f;
}

float Plu::GetRenderThreadFPS()
{
	const float delta = gRenderThreadDeltaTime.load(std::memory_order_relaxed);
	return delta > 0.0f ? 1.0f / delta : 0.0f;
}

void Plu::SetRenderFrameStats(UInt32 drawCalls, UInt32 instancesDrawn, UInt32 culledCount)
{
	gStatDrawCalls.store(drawCalls, std::memory_order_relaxed);
	gStatInstancesDrawn.store(instancesDrawn, std::memory_order_relaxed);
	gStatCulledCount.store(culledCount, std::memory_order_relaxed);
}

UInt32 Plu::GetStatDrawCalls()
{
	return gStatDrawCalls.load(std::memory_order_relaxed);
}

UInt32 Plu::GetStatInstancesDrawn()
{
	return gStatInstancesDrawn.load(std::memory_order_relaxed);
}

UInt32 Plu::GetStatCulledCount()
{
	return gStatCulledCount.load(std::memory_order_relaxed);
}

Plu::PathW Plu::GetEngineResourcesDir()
{
#ifdef PLU_PROJECT_ROOT
	return StringW::FromNarrow(PLU_PROJECT_ROOT);
#else
	return GetExePath().GetParentPath();
#endif
}

Plu::Path Plu::GetSystemUserPath()
{
#ifdef _WIN32
	const char* home = getenv("USERPROFILE");

	// Fallback: HOMEDRIVE + HOMEPATH (np. C: + \Users\Janek)
	if (!home) {
		const char* drive = getenv("HOMEDRIVE");
		const char* path  = getenv("HOMEPATH");

		if (drive && path) {
			String driveP(drive);
			String pathP(path);
			return driveP + pathP;
		}
		return "";
	}
#else
	const char* home = getenv("HOME");

	if (!home) return "";
#endif

	String pathS(home);
	return pathS;
}

Vec3 Plu::GetForwardVector(Vec3 rot)
{
	Quaternion qt = Quaternion(radians(rot));
	Vec3 forward = qt * Vec3(0, 0, -1);
	return glm::normalize(forward);
}

Vec3 Plu::GetRightVector(Vec3 rot)
{
	Quaternion qt = Quaternion(radians(rot));
	Vec3 right = qt * Vec3(1, 0, 0);
	return glm::normalize(right);
}

Vec3 Plu::GetUpVector(Vec3 rot)
{
	Quaternion qt = Quaternion(radians(rot));
	Vec3 up = qt * Vec3(0, 1, 0);
	return glm::normalize(up);
}

double Plu::ClampD(double val, double min, double max)
{
	return glm::clamp<double>(val, min, max);
}

float Plu::ClampF(float val, float min, float max)
{
	return glm::clamp<float>(val, min, max);
}

int Plu::ClampI(int val, int min, int max)
{
	return glm::clamp<int>(val, min, max);
}

float Plu::ClampAngle(float angle, float min, float max)
{
	angle = glm::mod(angle, 360.0f);
	if (angle > 180.0f)
		angle -= 360.0f;
	return glm::clamp(angle, min, max);
}

Vec3 Plu::GetLookAtRotatorDegrees(const Vec3 &eye, const Vec3 &target)
{

	Vec3 direction = glm::normalize(target - eye);

	float yawRad = std::atan2(direction.x, direction.z);

	float horizontalLength = std::sqrt(direction.x * direction.x + direction.z * direction.z);
	float pitchRad = std::atan2(direction.y, horizontalLength);

	float yawDegrees = glm::degrees(yawRad);
	float pitchDegrees = glm::degrees(pitchRad);

	while (yawDegrees > 180.0f) {
		yawDegrees -= 360.0f;
	}
	while (yawDegrees <= -180.0f) {
		yawDegrees += 360.0f;
	}

	return {
		pitchDegrees, // X (Pitch - góra/dół)
		yawDegrees,   // Y (Yaw - lewo/prawo)
		0.0f          // Z (Roll)
	};
}

Vec3 Plu::GetRotatedPointWithRadius(const Vec3 &center, float radius, float angleDegrees,
	const Vec3 &axis)
{
	// Krok 1: Definicja Punktu Początkowego (Na osi X lub innej, 
	// aby łatwo było określić kierunek).
	// Załóżmy, że punkt P zaczyna w (radius, 0, 0) względem centrum (0, 0, 0).
	glm::vec4 initialPoint = glm::vec4(radius, 0.0f, 0.0f, 1.0f);
    
	// Krok 2: Utwórz macierz transformacji
	glm::mat4 transform = glm::mat4(1.0f);

	// --- A. Wykonaj Obrot (Rotacja R) ---
	// Obracamy nasz "wektor promienia" (radius, 0, 0).
	float angleRadians = glm::radians(angleDegrees);
	transform = glm::rotate(transform, angleRadians, axis);

	// Krok 3: Zastosuj obrót
	glm::vec4 rotatedDirection = transform * initialPoint;

	// Krok 4: Przesunięcie do centrum
	// Dodajemy współrzędne centrum, aby przesunąć obrócony punkt z powrotem.
	// Zapewnia to, że punkt jest oddalony o 'radius' od 'center'.
	Vec3 finalPoint = center + Vec3(rotatedDirection);

	return finalPoint;
}

Vec3 Plu::GetSphericalOrbitPoint(const Vec3 &center, float radius, float yawDegrees, float pitchDegrees)
{
	float yawRad = glm::radians(yawDegrees);
	float pitchRad = glm::radians(pitchDegrees);

	Quaternion pitchRotation = glm::angleAxis(pitchRad, Vec3(1.0f, 0.0f, 0.0f)); // oś X
	Quaternion yawRotation   = glm::angleAxis(yawRad,   Vec3(0.0f, 1.0f, 0.0f)); // oś Y

	Quaternion combinedRotation = yawRotation * pitchRotation;

	Vec3 initialOffset = Vec3(0.0f, 0.0f, radius); // forward = Z
	Vec3 rotatedOffset = combinedRotation * initialOffset;

	return center + rotatedOffset;
}

void Plu::NormalizeVec3Rotation(Vec3 *vec)
{
	auto normalizeAngle = [](float angle) -> float
	{
		angle = glm::mod(angle, 360.0f);
		if (angle < 0.0f)
			angle += 360.0f;
		return angle;
	};

	*vec = Vec3(
		normalizeAngle(vec->x),
		normalizeAngle(vec->y),
		normalizeAngle(vec->z)
	);
}

Vec3 Plu::GetLocationFromMatrix(const Matrix4 &matrix)
{
	return Vec3(matrix[3]);
}

Vec3 Plu::GetScaleFromMatrix(const Matrix4 &matrix)
{
	return Vec3(
		glm::length(Vec3(matrix[0])),
		glm::length(Vec3(matrix[1])),
		glm::length(Vec3(matrix[2]))
	);
}

Vec3 Plu::GetRotationFromMatrix(const Matrix4 &matrix)
{
	Vec3 scale = GetScaleFromMatrix(matrix);
	glm::mat3 rotMat = glm::mat3(
		Vec3(matrix[0]) / scale.x,
		Vec3(matrix[1]) / scale.y,
		Vec3(matrix[2]) / scale.z
	);
	return glm::degrees(glm::eulerAngles(glm::quat_cast(rotMat)));
}

Plu::String Plu::MakeStringForDisplay(String text)
{
	static GameHashMap<String, String> stringCache;
	if (stringCache.Contains(text)) return stringCache[text];

	char lastChar = '\0';

	String display = "";

	for (auto c : text) {
		if (lastChar == '\0') {
			lastChar = c;
			display += c;
			continue;
		}
		if (std::isupper(c) && std::islower(lastChar)) {
			display += " ";
		}
		display += c;
		lastChar = c;
	}
	stringCache[text] = display;
	return display;
}

Plu::String Plu::PrepareCodeForDistribution(String code)
{
	std::string src = code.CStr();

	// Remove block comments /* */
	src = std::regex_replace(src, std::regex(R"(/\*[\s\S]*?\*/)"), "");

	std::istringstream stream(src);
	std::string result;
	std::string line;

	while (std::getline(stream, line)) {
		// Remove // comments
		auto commentPos = line.find("//");
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);

		// Trim surrounding whitespace
		auto lineStart = line.find_first_not_of(" \t\r");
		if (lineStart == std::string::npos) continue;
		line = line.substr(lineStart);
		auto lineEnd = line.find_last_not_of(" \t\r");
		if (lineEnd != std::string::npos) line = line.substr(0, lineEnd + 1);

		if (line.empty()) continue;

		if (line[0] == '#') {
			// Preprocessor directive — GLSL requires these to be on their own line.
			// Encode surrounding newlines as \n literal so Shaders.txt stays single-line.
			result += "\\n" + line + "\\n";
		} else {
			line = std::regex_replace(line, std::regex(R"(\s+)"), " ");
			result += line + " ";
		}
	}

	// Collapse multiple spaces that may appear at glue points
	result = std::regex_replace(result, std::regex(R"( {2,})"), " ");

	// Trim leading/trailing whitespace
	auto trimStart = result.find_first_not_of(" ");
	if (trimStart == std::string::npos) return "";
	result = result.substr(trimStart);
	while (!result.empty() && result.back() == ' ') result.pop_back();

	return result.c_str();
}
