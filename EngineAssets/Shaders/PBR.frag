#version 450 core

out vec4 FragColor;

// ==========================================================
// Wejścia z BasicVert.vert
// ==========================================================
in vec4 FragPos;     // pozycja fragmentu w world-space
in vec3 Normal;      // normalna geometryczna (world-space)
in vec2 TexCoord;    // UV
in vec3 VertColor;   // kolor wierzchołka
in mat3 TBN;         // baza tangent->world dla normal mappingu

// ==========================================================
// Uniformy silnika (engine-only — NIE są parametrami materiału)
// ==========================================================
uniform vec3 cameraPos;     // pozycja obserwatora (world-space)
uniform vec3 dirLightDir;   // kierunek LOTU światła (forward słońca; ku światłu = -dirLightDir)
uniform vec4 dirLightColor; // rgb = kolor, w = intensywność
uniform mat4 view;          // macierz widoku (do wyliczenia głębi w przestrzeni kamery — wybór kaskady)

// ==========================================================
// Cienie kaskadowe (CSM) światła kierunkowego — sterowane przez silnik
// (Renderer::RenderSnapshot). Mapy cieni zajmują sloty tekstur 0..CASCADE_COUNT-1;
// materiał dostaje swoje tekstury dopiero za nimi (SetSlotsUsed).
// ==========================================================
#define CASCADE_COUNT 4
uniform sampler2D cascadeShadowMaps[CASCADE_COUNT];
uniform mat4      cascadeLightSpaceMatrices[CASCADE_COUNT];
uniform float     cascadeSplitDistances[CASCADE_COUNT];
uniform int       cascadeCount;

// ==========================================================
// Parametry materiału (auto-wykrywane przez ShaderCodeParser)
// Tylko sampler2D / float / vec3 — pozostałe typy nie mają setterów w RenderFromMaterial.
// ==========================================================

// Mapy PBR
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D occlusionMap;

// Flagi użycia map (true = używaj mapy, false = używaj wartości skalarnej poniżej).
// Pozwala materiałowi bez przypisanej tekstury działać poprawnie (niepodpięty sampler
// nie jest bindowany przez RenderFromMaterial, więc bez tej flagi czytałby śmieci/slot 0).
uniform bool useAlbedoMap;
uniform bool useNormalMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useOcclusionMap;

// Wartości bazowe / mnożniki
uniform vec3  albedoColor;       // tint albedo (i fallback gdy brak mapy)
uniform float metallicFactor;    // [0..1]
uniform float roughnessFactor;   // [0..1]
uniform float occlusionStrength; // siła AO [0..1]
uniform float normalStrength;    // siła normal mappingu

// Prosty ambient zastępczy (brak IBL w silniku — patrz uwagi w PR).
uniform vec3  ambientColor;

const float PI = 3.14159265359;

// ----------------------------------------------------------
// Funkcje BRDF Cook-Torrance
// ----------------------------------------------------------

// Rozkład mikrofacetów (GGX / Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-6);
}

// Geometria — Schlick-GGX z aproksymacją k dla światła bezpośredniego
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith — uwzględnia geometrię od strony światła i obserwatora
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel — Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------
// Cień kaskadowy — zwraca widoczność światła [0..1] (1 = pełne światło, 0 = w cieniu).
// PCF 3x3. Konwencja identyczna jak w Shadow.frag.
// ----------------------------------------------------------
float ShadowVisibility(int cascade, vec3 worldPos, float bias)
{
    vec4 fragPosLS  = cascadeLightSpaceMatrices[cascade] * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords      = projCoords * 0.5 + 0.5;

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

void main()
{
    // --- Albedo (mapy są wgrywane jako linear, więc gdy używamy mapy sRGB->linear ręcznie) ---
    vec3 albedo = albedoColor;
    if (useAlbedoMap)
    {
        vec3 texel = texture(albedoMap, TexCoord).rgb;
        // Stopgap dopóki silnik nie wgrywa albedo w GL_SRGB8_ALPHA8.
        texel = pow(texel, vec3(2.2));
        albedo *= texel;
    }

    // --- Metallic / Roughness / AO ---
    float metallic  = metallicFactor;
    if (useMetallicMap)
        metallic *= texture(metallicMap, TexCoord).r;

    float roughness = roughnessFactor;
    if (useRoughnessMap)
        roughness *= texture(roughnessMap, TexCoord).r;
    roughness = clamp(roughness, 0.04, 1.0); // unikamy zerowej szorstkości

    float ao = 1.0;
    if (useOcclusionMap)
        ao = mix(1.0, texture(occlusionMap, TexCoord).r, occlusionStrength);

    // --- Normalna ---
    vec3 N;
    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalMap, TexCoord).xyz * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        N = normalize(TBN * tangentNormal);
    }
    else
    {
        N = normalize(Normal);
    }

    vec3 V = normalize(cameraPos - FragPos.xyz);

    // Reflektancja przy padaniu prostopadłym: dielektryki ~0.04, metale = albedo.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // ----------------------------------------------------------
    // Światło bezpośrednie — pojedyncze światło kierunkowe
    // ----------------------------------------------------------
    vec3 Lo = vec3(0.0);
    {
        vec3 L = normalize(-dirLightDir); // ku źródłu światła (dirLightDir = kierunek lotu promieni)
        vec3 H = normalize(V + L);
        vec3 radiance = dirLightColor.rgb * dirLightColor.w; // kolor * intensywność

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
        vec3 specular     = numerator / denominator;

        // Energia: kS = F, kD to reszta; metale nie mają dyfuzji.
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);

        // --- Cień kaskadowy dla światła kierunkowego ---
        // Wybór kaskady po głębi w przestrzeni widoku (dodatnia odległość od kamery).
        float shadow = 1.0;
        if (cascadeCount > 0)
        {
            float depthView = abs((view * FragPos).z);
            int cascade = cascadeCount - 1;
            for (int i = 0; i < cascadeCount - 1; i++) {
                if (depthView < cascadeSplitDistances[i]) {
                    cascade = i;
                    break;
                }
            }
            // Slope-scaled bias w znormalizowanej głębi [0,1]: większy przy stromym kącie padania.
            float bias = max(0.001 * (1.0 - NdotL), 0.00005);
            shadow = ShadowVisibility(cascade, FragPos.xyz, bias);
        }

        Lo += shadow * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient zastępczy (brak IBL) — AO przyciemnia tylko ambient.
    vec3 ambient = ambientColor * albedo * ao;

    vec3 color = ambient + Lo;

    // Tonemapping (Reinhard) + korekcja gamma — brak osobnego passa wyjściowego w silniku,
    // a bufor główny jest liniowy (RGBA8), więc robimy to tutaj.
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
