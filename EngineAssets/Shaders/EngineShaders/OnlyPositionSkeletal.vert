#version 450

// Depth-only shader dla shadow passa skeletal meshy — skinowany odpowiednik OnlyPosition.vert.
// Paleta kości ta sama co w BasicVertSkeletal.vert (SSBO binding 0), by cień pokrywał się z animacją.
layout(std430, binding = 0) buffer BoneMatrices {
    mat4 finalBoneMatrix[];
};

layout (location = 0) in vec3 aPos;
layout (location = 5) in ivec4 boneIDs;   // do 4 kości wpływających na wierzchołek
layout (location = 6) in vec4  boneWeights;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    mat4 skinMatrix =
          boneWeights.x * finalBoneMatrix[boneIDs.x]
        + boneWeights.y * finalBoneMatrix[boneIDs.y]
        + boneWeights.z * finalBoneMatrix[boneIDs.z]
        + boneWeights.w * finalBoneMatrix[boneIDs.w];

    gl_Position = lightSpaceMatrix * model * skinMatrix * vec4(aPos, 1.0);
}
