#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aVertColor;
// Tangent spakowany 10_10_10_2: xyz = tangent, w = znak bitangentu (handedness).
layout (location = 4) in vec4 aTangent;

out vec4 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 VertColor;
// Baza TBN w world-space dla normal mappingu (kolumny: tangent, bitangent, normal).
out mat3 TBN;

uniform mat4 model;
// Macierz normalnych policzona na CPU (transpose(inverse(model)), ustawiana przez Renderer
// razem z "model") — odporna na niejednorodne skalowanie, bez per-wierzchołkowego inverse()
// na GPU. Parytet z BasicVertInstanced.vert, gdzie idzie z SSBO InstanceMatrices.
uniform mat4 normalMatrix;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = model * vec4(aPos, 1.0);

    mat3 nMat = mat3(normalMatrix);

    vec3 N = normalize(nMat * aNormal);
    vec3 T = normalize(nMat * aTangent.xyz);
    // Re-ortogonalizacja Grama-Schmidta (T może nie być prostopadłe do N po interpolacji/skalowaniu).
    T = normalize(T - dot(T, N) * N);
    // Handedness z w decyduje o kierunku bitangentu (mirrored UV).
    vec3 B = cross(N, T) * aTangent.w;

    Normal = N;
    TBN = mat3(T, B, N);

    TexCoord = aTexCoord;
    VertColor = aVertColor;
}
