#version 450 core

out vec4 FragColor;

uniform vec3 BaseColor;
uniform float AmbientStrenght; 
uniform float SpecularStrength; 
uniform vec3 cameraPos;
uniform vec3 dirLightDir;
uniform vec4 dirLightColor;

in vec4 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 VertColor;

void main()
{
    vec3 ambient = BaseColor * AmbientStrenght;

    vec3 norm = normalize(Normal);

    float diff = max(dot(norm, dirLightDir), 0.0);
    vec3 diffuse = diff * dirLightColor.xyz;  

    vec3 viewDir = normalize(vec4(cameraPos, 0) - FragPos).xyz;
    vec3 reflectDir = reflect(-dirLightDir, norm);  

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = SpecularStrength * spec * dirLightColor.xyz;  

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}