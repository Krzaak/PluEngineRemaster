#version 330 core

uniform vec4 uuidColor;
out vec4 FragColor;

void main() {
    FragColor = uuidColor;
}