#version 450 core
// Interfejs zgodny z BasicVert.vert / BasicVertSkeletal.vert:
// FragPos to world-space vec4 (w == 1.0), głębię widokową liczymy tu z macierzy view.
in vec4 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

#define CASCADE_COUNT 5   // musi się zgadzać z Renderer::kCascadeCount
uniform sampler2D cascadeShadowMaps[CASCADE_COUNT];
uniform mat4      cascadeLightSpaceMatrices[CASCADE_COUNT];
uniform float     cascadeSplitDistances[CASCADE_COUNT];
uniform int       cascadeCount;

uniform mat4      view;

uniform vec3  dirLightDir;
uniform vec4  dirLightColor;

uniform vec3  AmbientColor;
uniform vec3  DiffuseColor;
uniform vec3  SpecularColor;
uniform float Specular;

// -------------------------------------------------------
// Rozmiar jednego teksela shadow-mapy w jednostkach świata dla danej kaskady.
// Macierz light-space to (ortho * lightView); ortho jest diagonalna, lightView ortonormalna,
// więc długość wiersza 0/1 macierzy 3x3 = skala NDC-na-świat wzdłuż osi X/Y (2/szerokość ortho).
// texel_świat = szerokość_ortho / rozdzielczość = 2 / (skala * rozdzielczość).
// res = textureSize mapy kaskady — liczone RAZ w ShadowCalculation i podawane parametrem
// (dawniej każda z 9 próbek PCF pytała textureSize od nowa).
float CascadeWorldTexelSize(int cascade, vec2 res)
{
    mat4  m    = cascadeLightSpaceMatrices[cascade];
    // Wiersze macierzy (glm jest column-major, więc wiersz i = m[0][i], m[1][i], m[2][i]).
    float sx   = length(vec3(m[0][0], m[1][0], m[2][0]));
    float sy   = length(vec3(m[0][1], m[1][1], m[2][1]));
    float tx   = 2.0 / (sx * res.x);
    float ty   = 2.0 / (sy * res.y);
    return max(tx, ty);
}

// Skala NDC-z na jednostkę świata dla danej kaskady (analogicznie do CascadeWorldTexelSize,
// tylko dla wiersza z). Dla ortho: sz = 2 / (far - near), więc bias w [0,1] głębi to
// biasWorld * sz * 0.5 — dzięki temu "1 cm biasu" znaczy 1 cm w KAŻDEJ kaskadzie, zamiast
// rosnąć z jej zakresem Z.
float CascadeDepthPerWorldUnit(int cascade)
{
    mat4 m = cascadeLightSpaceMatrices[cascade];
    return length(vec3(m[0][2], m[1][2], m[2][2])) * 0.5;
}

// Pojedyncza próbka cienia z bilinearnym POROWNANIEM (odpowiednik hardware'owego PCF na
// sampler2DShadow, ale bez zmiany stanu tekstury — depth mapy ogląda też TextureViewerPanel).
// textureGather zwraca 4 teksele kwadratu bilinearnego; porównujemy każdy z osobna i ważymy
// wynik ułamkiem pozycji. Różnica vs twarde porównanie + uśrednianie: kilkutekselowy cień
// małego obiektu zachowuje ciemny środek i ostrą, subtekselową krawędź zamiast szarej papki.
float SampleShadowBilinear(int cascade, vec2 uv, float refDepth, vec2 res)
{
    vec2 st   = uv * res - 0.5;
    vec2 base = floor(st);
    vec2 f    = st - base;

    // Gather w środku kwadratu [base, base+1]: w=(0,0), z=(1,0), x=(0,1), y=(1,1).
    vec4 d   = textureGather(cascadeShadowMaps[cascade], (base + 1.0) / res, 0);
    vec4 lit = step(vec4(refDepth), d);   // 1 = oświetlony (depth mapy >= głębia fragmentu)

    return mix(mix(lit.w, lit.z, f.x), mix(lit.x, lit.y, f.x), f.y);
}

// Normal-offset + PCF. Główny ciężar anty-acne niesie glPolygonOffset w passie castera
// (Renderer.cpp, RenderShadowPass) — per-trójkąt, w jednostkach precyzji bufora głębi,
// niezależnie od kaskady. Tu zostają korekty po stronie odbiorcy i WSZYSTKIE mają cap
// w jednostkach świata: skalowanie czysto tekselowe rosło z grubością kaskady i w dalekich
// kaskadach (teksel ~kilkanaście cm) zjadało cienie małych obiektów.
float ShadowCalculation(int cascade, vec3 worldPos, vec3 normal, float slope)
{
    vec2  res        = vec2(textureSize(cascadeShadowMaps[cascade], 0));
    float worldTexel = CascadeWorldTexelSize(cascade, res);

    // Normal-offset: pół teksela + teksel na slope, ale nigdy więcej niż ~8 cm świata —
    // powyżej tego w grubych kaskadach próbka wyjeżdżała poza cień małych obiektów.
    float offsetScale = min(worldTexel * (0.5 + 1.0 * slope), 0.08);
    vec3  offsetPos   = worldPos + normal * offsetScale;

    vec4 fragPosLS   = cascadeLightSpaceMatrices[cascade] * vec4(offsetPos, 1.0);
    vec3 projCoords  = fragPosLS.xyz / fragPosLS.w;
    projCoords       = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    // Depth-bias w jednostkach świata (0.5 cm + do ~5 cm przy ślizgu), przeliczony na [0,1]
    // głębię TEJ kaskady — stały koszt w metrach niezależnie od zakresu Z kaskady.
    float depthBias    = (0.005 + 0.01 * slope) * CascadeDepthPerWorldUnit(cascade);
    float currentDepth = projCoords.z - depthBias;

    // PCF o stałej SZEROKOŚCI ŚWIATA (~2 cm penumbry), nie stałej liczbie tekseli:
    // w bliskiej kaskadzie (teksel ~5 mm) daje pełne 3x3 ±1 teksel, w dalekiej (teksel
    // kilkanaście cm) taps zbiegają do <pół teksela — bilinearne porównanie i tak wygładza
    // krawędź, a mały cień nie jest rozmywany o ±15 cm.
    // Górny clamp 0.75 teksela (nie 1.0): bilinearne porównanie samo dokłada ~1 teksel
    // wygładzenia, więc ciaśniejsze tapy = ostrzejsza krawędź z bliska bez schodków.
    vec2  texelSize    = 1.0 / res;
    float radiusTexels = clamp(0.02 / worldTexel, 0.35, 0.75);

    float shadow = 0.0;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 offs = vec2(x, y) * radiusTexels * texelSize;
        shadow += SampleShadowBilinear(cascade, projCoords.xy + offs, currentDepth, res);
    }
    return shadow / 9.0;
}

// -------------------------------------------------------
void main()
{
    vec3  normal = normalize(Normal);
    vec3  lightD = normalize(-dirLightDir);
    float nDotL  = max(dot(normal, lightD), 0.0);
    // tan(θ) kąta padania światła — wymagany bias przeciw acne rośnie właśnie jak tan, a nie
    // liniowo: przy kącie ślizgowym (nDotL→0) dąży do nieskończoności. Clampujemy nDotL od dołu,
    // by tan nie wybuchł, i tniemy slope do 5 (poza tym powierzchnia i tak jest ledwie oświetlona).
    float cosT   = max(nDotL, 0.1);
    float slope  = min(sqrt(1.0 - cosT * cosT) / cosT, 5.0);   // 0 (prosto) .. 5 (ślizg)

    // Anty-acne: główny mechanizm to slope-scaled glPolygonOffset w passie castera
    // (per-trójkąt, w jednostkach bufora głębi — niezależny od rozmiaru kaskady).
    // Korekty po stronie odbiorcy (normal-offset i depth-bias, obie z capem w jednostkach
    // świata) liczą się w ShadowCalculation, bo zależą od kaskady.

    // Głębia w przestrzeni widoku — dawniej wyliczana w Shadow.vert, teraz z world-space FragPos.
    // Trzeci wiersz view × FragPos: potrzebna tylko składowa z, bez pełnego mnożenia mat4 × vec4.
    float FragDepthViewSpace = abs(dot(vec4(view[0][2], view[1][2], view[2][2], view[3][2]), FragPos));

    // Wybierz kaskadę na podstawie głębokości w przestrzeni widoku
    int cascade = cascadeCount - 1;
    for (int i = 0; i < cascadeCount - 1; i++) {
        if (FragDepthViewSpace < cascadeSplitDistances[i]) {
            cascade = i;
            break;
        }
    }

    float shadow = ShadowCalculation(cascade, FragPos.xyz, normal, slope);

    vec3  ambient    = dirLightColor.rgb * AmbientColor;
    float diffFactor = max(dot(normal, lightD), 0.0);
    vec3  diffuse    = dirLightColor.rgb * DiffuseColor * diffFactor;
    vec3  H          = normalize(lightD + normal);
    float specFactor = pow(max(dot(H, normal), 0.0), Specular);
    vec3  specular   = dirLightColor.rgb * SpecularColor * specFactor;

    FragColor = vec4(ambient + shadow * (diffuse + specular), 1.0);
}
