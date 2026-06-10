#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D dirShadowMap;
uniform sampler2D diffuseTexture;
uniform vec3 dirLightDir;    // znormalizowany, od fragmentu DO światła

// -------------------------------------------------------
float ShadowCalculation(vec4 fragPosLS, float bias)
{
    // Perspektywiczny podział → NDC
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5; // [−1,1] → [0,1]

    // Poza frustumem światła = brak cienia
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    // PCF — wygładzone cienie (3x3 kernel)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(dirShadowMap, 0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float pcfDepth = texture(dirShadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
    return shadow / 9.0;
}

// -------------------------------------------------------
void main()
{
    vec3 color    = texture(diffuseTexture, TexCoord).rgb;
    vec3 normal   = normalize(Normal);
    vec3 lightD   = normalize(dirLightDir);

    // Bias adaptacyjny — redukuje shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightD)), 0.0005);

    float shadow  = ShadowCalculation(FragPosLightSpace, bias);

    // Proste oświetlenie diffuse
    float diff    = max(dot(normal, lightD), 0.0);
    vec3 ambient  = 0.15 * color;
    vec3 diffuse  = diff * color;

    vec3 result = ambient + (1.0 - shadow) * diffuse;
    FragColor = vec4(result, 1.0);
}