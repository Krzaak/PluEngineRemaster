//
// Created by Plutex on 11.01.2026.
//

#ifndef PLUENGINE_PLUTYPES_H
#define PLUENGINE_PLUTYPES_H

#include "glm/fwd.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"
#include "glm/detail/type_quat.hpp"
#include "nlohmann/json.hpp"

typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;

typedef glm::ivec2 IVec2;
typedef glm::ivec3 IVec3;
typedef glm::ivec4 IVec4;

typedef glm::mat4 Matrix4;

typedef glm::quat Quaternion;

typedef nlohmann::json JSON;

#endif //PLUENGINE_PLUTYPES_H