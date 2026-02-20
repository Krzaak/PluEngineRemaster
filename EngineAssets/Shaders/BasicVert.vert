#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aVertColor;

out vec4 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 VertColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = model * vec4(aPos, 1.0);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    VertColor = aVertColor;
}
