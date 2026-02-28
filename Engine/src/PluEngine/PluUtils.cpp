//
// Created by Plutex on 2026-02-28.
//

#include "PluEngine/PluUtils.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

Vec3 Plu::GetForwardVector(Vec3 rot)
{
	glm::quat qt = glm::quat(radians(rot));
	glm::vec3 forward = qt * glm::vec3(0, 0, -1);
	return glm::normalize(forward);
}

Vec3 Plu::GetRightVector(Vec3 rot)
{
	glm::quat qt = glm::quat(radians(rot));
	glm::vec3 right = qt * glm::vec3(1, 0, 0);
	return glm::normalize(right);
}

Vec3 Plu::GetUpVector(Vec3 rot)
{
	glm::quat qt = glm::quat(radians(rot));
	glm::vec3 up = qt * glm::vec3(0, 1, 0);
	return glm::normalize(up);
}
