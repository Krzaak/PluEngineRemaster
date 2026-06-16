#version 450 core
layout (location = 0) in vec3 aPos;    // Pozycja w modelu
layout (location = 1) in vec3 aNormal; // Normalna do powierzchni

// Macierze transformacji
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Wyjście do Fragment Shader
out vec3 FragPosWorld;
out vec3 NormalWorld;

void main()
{
    // Obliczanie pozycji w świecie (World Space)
    FragPosWorld = vec3(model * vec4(aPos, 1.0));

    // Obliczanie normalnej w świecie (World Space).
    // Ważne: Normalne skalują się, więc używamy macierzy transponowanej (transpose)
    // lub bezpieczniej: transformacji 3x3.
    NormalWorld = mat3(model) * aNormal;

    // Obliczanie ostatecznej pozycji (dla transformacji viewport)
    gl_Position = projection * view * vec4(vec3(model * vec4(aPos, 1.0)), 1.0);
}