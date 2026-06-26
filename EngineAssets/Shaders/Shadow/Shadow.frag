#version 450 core
in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoord;
in float FragDepthViewSpace;

out vec4 FragColor;

#define CASCADE_COUNT 4
uniform sampler2D cascadeShadowMaps[CASCADE_COUNT];
uniform mat4      cascadeLightSpaceMatrices[CASCADE_COUNT];
uniform float     cascadeSplitDistances[CASCADE_COUNT];
uniform int       cascadeCount;

uniform vec3  dirLightDir;
uniform vec4  dirLightColor;

uniform vec3  AmbientColor;
uniform vec3  DiffuseColor;
uniform vec3  SpecularColor;
uniform float Specular;

// -------------------------------------------------------
float ShadowCalculation(int cascade, float bias)
{
    vec4 fragPosLS   = cascadeLightSpaceMatrices[cascade] * vec4(FragPos, 1.0);
    vec3 projCoords  = fragPosLS.xyz / fragPosLS.w;
    projCoords       = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    float currentDepth = projCoords.z;
    vec2  texelSize    = 1.0 / vec2(textureSize(cascadeShadowMaps[cascade], 0));
    float shadow       = 0.0;

    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float pcfDepth = texture(cascadeShadowMaps[cascade], projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
    }
    return shadow / 9.0;
}

// -------------------------------------------------------
void main()
{
    vec3  normal = normalize(Normal);
    vec3  lightD = normalize(-dirLightDir);
    // Slope-scaled bias: większy przy stromym kącie padania światła, mały gdy światło pada prosto.
    // Wartości są w znormalizowanej głębi [0,1] — celowo małe, by cień nie odrywał się od obiektu.
    float bias   = max(0.001 * (1.0 - dot(normal, lightD)), 0.00005);

    // Wybierz kaskadę na podstawie głębokości w przestrzeni widoku
    int cascade = cascadeCount - 1;
    for (int i = 0; i < cascadeCount - 1; i++) {
        if (FragDepthViewSpace < cascadeSplitDistances[i]) {
            cascade = i;
            break;
        }
    }
    //Hello
    float shadow = ShadowCalculation(cascade, bias);

    vec3  ambient    = dirLightColor.rgb * AmbientColor * 0.2;
    float diffFactor = max(dot(normal, lightD), 0.0);
    vec3  diffuse    = dirLightColor.rgb * DiffuseColor * diffFactor;
    vec3  H          = normalize(lightD + normal);
    float specFactor = pow(max(dot(H, normal), 0.0), Specular);
    vec3  specular   = dirLightColor.rgb * SpecularColor * specFactor;

    FragColor = vec4(ambient + shadow * (diffuse + specular), 1.0);
}
