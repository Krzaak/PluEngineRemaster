//
// Created by Plutex on 2026-02-28.
//

#include "PluEngine/PluUtils.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

Plu::PathW Plu::GetEngineResourcesDir()
{
#ifdef PLU_PROJECT_ROOT
	return StringW::FromNarrow(PLU_PROJECT_ROOT);
#elif
	return GetExePath().GetParentPath();
#endif
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

Vec3 Plu::GetLookAtRotatorDegrees(const Vec3 &eye, const Vec3 &target)
{
    
	// 1. Oblicz wektor kierunku (od oka do celu)
	Vec3 direction = glm::normalize(target - eye);
    
	// 2. Oblicz YAW (obrót wokół osi Y)
	float yawRad = std::atan2(direction.x, direction.z);

	// 3. Oblicz PITCH (obrót wokół osi X)
	float horizontalLength = std::sqrt(direction.x * direction.x + direction.z * direction.z);
	float pitchRad = std::atan2(direction.y, horizontalLength);

	// 4. Konwersja na stopnie
	float yawDegrees = glm::degrees(yawRad);
	float pitchDegrees = glm::degrees(pitchRad);

	// 5. KOREKTA 180 STOPNI (TYPOWA DLA TEGO TYPU OBLICZEŃ)
	// Dodajemy 180 stopni do YAW, aby "odwrócić" kierunek, 
	// który patrzył na cel, na kierunek z którego cel jest widziany.
	// Upewniamy się, że kąt jest w zakresie (-180, 180] (lub 0-360, jeśli wolisz).
	yawDegrees += 180.0f;
    
	// Opcjonalnie: Normalizacja do zakresu (-180, 180]
	while (yawDegrees > 180.0f) {
		yawDegrees -= 360.0f;
	}
	while (yawDegrees <= -180.0f) {
		yawDegrees += 360.0f;
	}

	// Zwracamy wektor (Pitch, Yaw, Roll)
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
	// --- Krok 1: Konwersja na Radiany ---
	float yawRad = glm::radians(yawDegrees);
	float pitchRad = glm::radians(pitchDegrees);

	// --- Krok 2: Utworzenie Kwaternionów i Łączenie Obrotów (Dwie Naraz) ---
	// Najpierw Pitch (obrót pionowy wokół X), potem Yaw (obrót poziomy wokół Y).
	// Kolejność jest kluczowa: Yaw (globalny) * Pitch (lokalny)
    
	// Kwaternion Pitch (obrót wokół osi X)
	Quaternion pitchRotation = glm::angleAxis(pitchRad, Vec3(0.0f, 0.0f, 1.0f));
    
	// Kwaternion Yaw (obrót wokół osi Y)
	Quaternion yawRotation = glm::angleAxis(yawRad, Vec3(0.0f, 1.0f, 0.0f));
    
	// Łączenie obrotów (obie naraz):
	// Zastosuj najpierw Pitch, a następnie Yaw (Pitch jest "wmontowany" w Yaw)
	Quaternion combinedRotation = yawRotation * pitchRotation; 
    
	// --- Krok 3: Obrót Wektora Promienia ---
	// Punkt początkowy (przesunięcie) w układzie lokalnym. 
	// Zaczynamy od (radius, 0, 0)
	Vec3 initialOffset = Vec3(radius, 0.0f, 0.0f);
    
	// Obracamy wektor przesunięcia połączonym kwaternionem
	Vec3 rotatedOffset = combinedRotation * initialOffset;

	// --- Krok 4: Przesunięcie do Centrum ---
	// Dodajemy obrócony wektor przesunięcia do centrum, 
	// aby uzyskać ostateczną pozycję w świecie.
	Vec3 finalPoint = center + rotatedOffset;

	return finalPoint;
}
