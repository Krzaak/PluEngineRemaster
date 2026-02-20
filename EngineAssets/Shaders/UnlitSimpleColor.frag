#version 450 core

out vec4 FragColor;

uniform vec3 BaseColor;
uniform int DebugMode; //0 - Color, 1 - Normals, 2 - UVs, 3 - Depth, 4 - Vertex Colors, 5 - Wireframe
uniform sampler2D ColorTexture;

in vec4 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 VertColor;

void main()
{
    switch (DebugMode)
    {
        case 0: // Color
            FragColor = vec4(BaseColor, 1.0);
            break;
        case 1: // Normals
            FragColor = vec4(normalize(Normal.xyz) * 0.5 + 0.5, 1.0);
            break;
        case 2: // UVs
            FragColor = vec4(TexCoord, 0.0, 1.0); // TexCoord should be passed from vertex shader
            break;
        case 3: // Depth
            float depth = FragPos.z / FragPos.w; // Perspective divide
            depth = depth * 0.5 + 0.5; // Normalize to [0,1]
            FragColor = vec4(vec3(depth), 1.0);
            break;
        case 4: // Vertex Colors
            FragColor = vec4(VertColor, 1.0); // Vertex color should be passed from vertex shader
            break;
        case 5: // Wireframe
            FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Placeholder for wireframe color
            break;
        case 6:
            FragColor = texture(ColorTexture, TexCoord);
            break;
        default:
            FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta for undefined mode
            break;
    }
}
