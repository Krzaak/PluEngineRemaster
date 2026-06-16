//
// Created by Plutex on 2026-02-28.
//

#include "PluEngine/PluUtils.h"

#include <regex>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

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
	std::string result = code.CStr();

	// Usuń komentarze //
	result = std::regex_replace(result, std::regex(R"(//[^\n]*)"), "");

	// Usuń komentarze /* */
	result = std::regex_replace(result, std::regex(R"(/\*[\s\S]*?\*/)"), "");

	// Zamień wielokrotne whitespace (spacje, taby, newliny) na jedną spację
	result = std::regex_replace(result, std::regex(R"(\s+)"), " ");

	// Trim
	if (!result.empty() && result.front() == ' ') result.erase(0, 1);
	if (!result.empty() && result.back() == ' ')  result.pop_back();

	return result.c_str();
}
