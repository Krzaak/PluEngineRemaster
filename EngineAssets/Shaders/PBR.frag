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
// (Renderer::RenderSnapshot). Wszystkie kaskady leżą w JEDNEJ teksturze GL_TEXTURE_2D_ARRAY
// na slocie 15 — OSTATNIM gwarantowanym przez GL 4.5 (GL_MAX_TEXTURE_IMAGE_UNITS >= 16).
//
// Dlaczego nie na slocie 0: sampler materiału, któremu nie przypisano tekstury, NIE dostaje
// przypisanego slotu (RenderFromMaterial wywołuje SetTextureUniform tylko dla realnie
// zbindowanych tekstur), więc zostaje na domyślnym slocie 0. Dwa samplery RÓŻNYCH typów
// (sampler2D materiału + sampler2DArrayShadow) na tym samym slocie to INVALID_OPERATION przy
// rysowaniu — część sterowników to toleruje, część (Mesa) nie i gasi cały pass. Trzymanie
// cieni na drugim końcu zakresu usuwa całą klasę tych kolizji.
// ==========================================================
// MAX_CASCADE_COUNT jest tylko górnym ograniczeniem tablic (Plu::kMaxShadowCascades);
// realną liczbę kaskad niesie cascadeCount z bloku ShadowData. Indeksowanie tablic w UBO
// zmienną nie-uniformową jest legalne (w odróżnieniu od tablic samplerów — tamto było
// formalnym UB, dlatego kaskady są dziś warstwami jednej tekstury, nie N samplerami).
#define MAX_CASCADE_COUNT 4
layout(std140, binding = 2) uniform ShadowData
{
    mat4  cascadeViewProj[MAX_CASCADE_COUNT];
    vec4  cascadeSplits;          // odległość widokowa końca kaskady, per komponent
    vec4  cascadeTexelSizes;      // rozmiar teksela w metrach, per kaskada
    vec4  cascadeDepthBias;       // bias w [0,1] głębi kaskady, policzony na CPU
    int   cascadeCount;           // 0 = brak cieni kierunkowych w tej klatce
    float shadowFadeStart;        // metry — od tąd cień zanika
    float shadowFadeEnd;          // metry — za tym dystansem pełne światło
    float cascadeBlendFraction;   // jaka część kaskady służy do przenikania w następną
    float normalBiasScale;        // normal-offset w tekselach
    float pcfRadiusTexels;        // promień dysku PCF w tekselach
    int   debugVisualizeCascades; // != 0 = koloruj kaskady
    float invShadowMapResolution; // 1 / rozdzielczość mapy cienia
    int   pcfTapCount;            // liczba próbek dysku PCF (1 = jedno sprzętowe pobranie)
    int   pcfRotateSamples;       // != 0 = obrót dysku per piksel (schodki -> dither)
};

// Górny limit pętli filtra; musi odpowiadać Plu::kMaxShadowPcfTaps.
#define MAX_PCF_TAPS 32

// Sampler PORÓWNUJĄCY: `texture()` na sampler2DArrayShadow zwraca wynik porównania głębi
// przefiltrowany sprzętowo (2x2 PCF za jedno pobranie), a nie samą głębię. Tryb porównania
// siedzi na obiekcie samplera bindowanym tylko na czas passu światła (Renderer), więc sama
// tekstura zostaje zwykłą teksturą głębi — podglądaną np. przez TextureViewerPanel.
layout(binding = 15) uniform sampler2DArrayShadow shadowCascades;

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
//
// Budżet biasu (świadomie rozdzielony, bez literałów w GLSL):
//  * caster — slope-scaled glPolygonOffset + front-face culling w passie cieni (Renderer.cpp);
//    działa per trójkąt, w jednostkach precyzji bufora głębi, więc nie przesuwa cienia w bok;
//  * receiver — normal-offset w TEKSELACH tej kaskady (niżej) + depth-bias w metrach
//    przeliczony na CPU na [0,1] głębi kaskady (cascadeDepthBias z UBO).
// ----------------------------------------------------------

// Dysk Vogela: `count` próbek rozłożonych po złotym kącie. W przeciwieństwie do stałej tablicy
// Poissona jest liczony analitycznie, więc pcfTapCount jest ciągłym suwakiem jakości — przy 8
// próbkach daje ten sam rozkład co dawny dysk, przy 16-32 wypełnia promień bez dziur.
vec2 VogelDiskSample(int index, int count, float phase)
{
    const float kGoldenAngle = 2.39996323;
    float radius = sqrt((float(index) + 0.5) / float(count));
    float theta  = float(index) * kGoldenAngle + phase;
    return radius * vec2(cos(theta), sin(theta));
}

// Interleaved gradient noise (Jimenez) — deterministyczna funkcja współrzędnej piksela.
// Obraca dysk per piksel: krawędź, która przy stałym wzorze układa się w schodki wielkości
// teksela kaskady, rozsypuje się na dither o szerokości jednego piksela ekranu. To jedyny
// sposób, żeby MAŁY promień PCF (ostry cień) nie wyglądał na pikselowaty.
float InterleavedGradientNoise(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

// Jedno sprzętowe porównanie: zwraca przefiltrowany bilinearnie wynik (refDepth <= głębia mapy),
// czyli 2x2 PCF za JEDNO pobranie. Dzięki temu każdy tap dysku jest już sub-tekselowy, a N tapów
// daje ~4N efektywnych porównań z N pobrań.
float TapShadow(int cascade, vec2 uv, float refDepth)
{
    return texture(shadowCascades, vec4(uv, float(cascade), refDepth));
}

float FilterCascade(int cascade, vec3 worldPos, vec3 normal, float slope)
{
    // Normal-offset liczony w tekselach TEJ kaskady: to jedyna skala, w której "przesuń próbkę
    // poza własny cień" znaczy to samo w kaskadzie bliskiej i dalekiej.
    float texelWorld  = cascadeTexelSizes[cascade];
    float offsetScale = texelWorld * normalBiasScale * clamp(0.5 + slope, 0.5, 3.0);
    vec3  offsetPos   = worldPos + normal * offsetScale;

    // Projekcja kaskady jest ortho, więc w == 1 — dzielenie perspektywiczne jest zbędne.
    vec4 fragPosLS  = cascadeViewProj[cascade] * vec4(offsetPos, 1.0);
    vec3 projCoords = fragPosLS.xyz * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;
    // Brak testu zakresu UV: tekstura ma CLAMP_TO_BORDER z borderem 1.0, więc poza mapą
    // porównanie zawsze wypada "oświetlony".

    float refDepth = projCoords.z - cascadeDepthBias[cascade] * (1.0 + 2.0 * slope);

    // Jedno pobranie to już sprzętowy 2x2 PCF, więc przy zerowym promieniu (albo jednej próbce)
    // dysk jest zbędny — to najostrzejszy możliwy cień, ograniczony wyłącznie gęstością tekseli.
    int taps = clamp(pcfTapCount, 1, MAX_PCF_TAPS);
    if (taps == 1 || pcfRadiusTexels <= 0.0)
    {
        return TapShadow(cascade, projCoords.xy, refDepth);
    }

    vec2  radiusUV = vec2(pcfRadiusTexels * invShadowMapResolution);
    float phase    = (pcfRotateSamples != 0)
                   ? InterleavedGradientNoise(gl_FragCoord.xy) * 6.28318530
                   : 0.0;

    float shadow = 0.0;
    for (int i = 0; i < taps; i++)
    {
        shadow += TapShadow(cascade, projCoords.xy + VogelDiskSample(i, taps, phase) * radiusUV, refDepth);
    }
    return shadow / float(taps);
}

// Wybór kaskady po głębi w przestrzeni widoku (dodatnia odległość od kamery).
int SelectCascade(float depthView)
{
    int cascade = cascadeCount - 1;
    for (int i = 0; i < cascadeCount - 1; i++)
    {
        if (depthView < cascadeSplits[i]) { return i; }
    }
    return cascade;
}

float ShadowVisibility(vec3 worldPos, vec3 normal, float depthView, float slope)
{
    if (cascadeCount <= 0) return 1.0;

    int   cascade    = SelectCascade(depthView);
    float visibility = FilterCascade(cascade, worldPos, normal, slope);

    // Przenikanie kaskad: na ostatnim cascadeBlendFraction zakresu kaskady mieszamy jej wynik
    // z wynikiem następnej. Bez tego skok rozmiaru teksela rysuje wyraźny szew w poprzek sceny.
    if (cascade < cascadeCount - 1 && cascadeBlendFraction > 0.0)
    {
        float rangeStart = (cascade == 0) ? 0.0 : cascadeSplits[cascade - 1];
        float rangeEnd   = cascadeSplits[cascade];
        float bandStart  = mix(rangeEnd, rangeStart, cascadeBlendFraction);
        float t = clamp((depthView - bandStart) / max(rangeEnd - bandStart, 1e-4), 0.0, 1.0);
        if (t > 0.0)
        {
            visibility = mix(visibility, FilterCascade(cascade + 1, worldPos, normal, slope), t);
        }
    }

    // Wygaszanie na dystansie: zamiast twardej krawędzi na końcu ostatniej kaskady cień
    // rozpływa się w pełne światło.
    float fade = clamp((depthView - shadowFadeStart) / max(shadowFadeEnd - shadowFadeStart, 1e-4), 0.0, 1.0);
    return mix(visibility, 1.0, fade);
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

    // Głębia w przestrzeni widoku — trzeci wiersz macierzy view × FragPos (potrzebna tylko
    // składowa z, bez pełnego mnożenia mat4 × vec4). Steruje wyborem kaskady i debug tintem,
    // więc liczona przed gałęzią światła.
    float depthView = abs(dot(vec4(view[0][2], view[1][2], view[2][2], view[3][2]), FragPos));

    // ----------------------------------------------------------
    // Światło bezpośrednie — pojedyncze światło kierunkowe
    // ----------------------------------------------------------
    vec3 Lo = vec3(0.0);
    {
        vec3 L = normalize(-dirLightDir); // ku źródłu światła (dirLightDir = kierunek lotu promieni)
        float NdotL = max(dot(N, L), 0.0);

        // Early-out: cały wkład światła bezpośredniego (BRDF + próbkowanie cienia w
        // ShadowVisibility) jest na końcu mnożony przez NdotL, więc dla fragmentów odwróconych
        // od światła (NdotL == 0, ~połowa każdego obiektu) wynik to gwarantowane zero — nie licz
        // niczego. Wynik identyczny co do bitu.
        if (NdotL > 0.0)
        {
            vec3 H = normalize(V + L);
            vec3 radiance = dirLightColor.rgb * dirLightColor.w; // kolor * intensywność

            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness);
            vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 numerator    = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 1e-4;
            vec3 specular     = numerator / denominator;

            // Energia: kS = F, kD to reszta; metale nie mają dyfuzji.
            vec3 kS = F;
            vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

            // --- Cień kaskadowy dla światła kierunkowego ---
            // slope = tan(θ) kąta padania — wymagany bias rośnie właśnie jak tan, a nie liniowo:
            // przy kącie ślizgowym (NdotL→0) dąży do nieskończoności. Clampujemy cos od dołu, by
            // tan nie wybuchł, i tniemy slope do 5 (poza tym powierzchnia i tak ledwie świeci).
            float cosT  = max(NdotL, 0.1);
            float slope = min(sqrt(1.0 - cosT * cosT) / cosT, 5.0);
            float shadow = ShadowVisibility(FragPos.xyz, N, depthView, slope);

            Lo += shadow * (kD * albedo / PI + specular) * radiance * NdotL;
        }
    }

    // Ambient zastępczy (brak IBL) — AO przyciemnia tylko ambient.
    vec3 ambient = ambientColor * albedo * ao;

    vec3 color = ambient + Lo;

    // Debug: pokoloruj pikselom przypisaną kaskadę (View → ustawienia sceny). Strefy przenikania
    // widać jako gradient między sąsiednimi pasami.
    if (debugVisualizeCascades != 0 && cascadeCount > 0)
    {
        const vec3 kCascadeTint[MAX_CASCADE_COUNT] = vec3[](
            vec3(1.0, 0.4, 0.4), vec3(0.4, 1.0, 0.4),
            vec3(0.4, 0.6, 1.0), vec3(1.0, 1.0, 0.4)
        );
        color *= kCascadeTint[SelectCascade(depthView)];
    }

    // Tonemapping (Reinhard) + korekcja gamma — brak osobnego passa wyjściowego w silniku,
    // a bufor główny jest liniowy (RGBA8), więc robimy to tutaj.
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
