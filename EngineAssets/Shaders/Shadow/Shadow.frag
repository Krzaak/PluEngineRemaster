#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D dirShadowMap;
uniform vec3 dirLightDir;
uniform vec4 dirLightColor;

uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess; 

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
    vec2 texelSize = textureSize(dirShadowMap, 0) / 2;
    for (int x = -3; x <= 3; x++)
    for (int y = -3; y <= 3; y++)
    {
        float pcfDepth = texture(dirShadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
    }
    return shadow / 49.0;
    //
}

// -------------------------------------------------------
void main()
{
    vec3 normal   = normalize(Normal);
    vec3 lightD   = normalize(-dirLightDir);

    // Bias adaptacyjny — redukuje shadow acne
    float bias = max(0.004 * (0.7 - dot(normal, lightD)), 0.016);

    float shadow  = ShadowCalculation(FragPosLightSpace, bias);

    vec3 ambient = dirLightColor.rgb * materialAmbient * 0.2;

    float diffFactor = max(dot(normal, lightD), 0.0); // max(..., 0.0) zapobiega negatywnym kolorom.
    vec3 diffuse = dirLightColor.rgb * materialDiffuse * diffFactor;

    vec3 H = normalize(lightD + normal);

    float specFactor = pow(max(dot(H, normal), 0.0), materialShininess);
    vec3 specular = dirLightColor.rgb * materialSpecular * specFactor;

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(shadow == 1 ? result : ambient, 1.0);
}