#version 450 core
out vec4 FragColor;

// Dane otrzymane z Vertex Shader
in vec3 FragPosWorld;
in vec3 NormalWorld;

// ==========================================================
// UNIFORMY DLA OŚWIETLENIA (WYMAGANE)
// ==========================================================

// Kierunek światła (musi być znormalizowane)
uniform vec3 dirLightDir;
// Kolor i Intensywność światła (np. R=kolor, G=intensywność)
uniform vec4 dirLightColor;

// ==========================================================
// UNIFORMY DLA MATERIAŁU I SCENY
// ==========================================================

// Kolor podstawowy obiektu (np. mat_diffuse * vec4(1.0))
uniform vec3 materialAmbient; // Kolor ambient
uniform vec3 materialDiffuse; // Kolor rozproszony
uniform vec3 materialSpecular; // Kolor odbicia
uniform float materialShininess; // Połysk (wysokiej liczby = mały błysk)

void main()
{
    // 1. Normalizacja wektorów (CRITICAL!)
    // Musimy upewnić się, że wszystkie wektory używane w obliczeniach dot product są jednostkowe.
    vec3 N = normalize(NormalWorld);

    // DirLightDir już powinno być znormalizowane, ale zawsze lepiej to sprawdzić:
    vec3 L = normalize(dirLightDir);

    // Obliczanie światła

    // A. Oświetlenie Ambient (Stały, podstawowy kolor)
    vec3 ambient = dirLightColor.rgb * materialAmbient * 0.2;

    // B. Oświetlenie Diffuse (Rozproszone)
    // Maksymalne światło jest tam, gdzie N i L są zbieżne (dot product = 1).
    float diffFactor = max(dot(N, L), 0.0); // max(..., 0.0) zapobiega negatywnym kolorom.
    vec3 diffuse = dirLightColor.rgb * materialDiffuse * diffFactor;

    // C. Oświetlenie Specular (Odbicie/Błysk)
    // 1. Obliczanie wektora Halfway (H): Średnia między N i L (w idealnym przypadku to N, L, R)
    vec3 H = normalize(L + N);

    // 2. Obliczanie siły specular: cos(theta)^n
    float specFactor = pow(max(dot(H, N), 0.0), materialShininess);
    vec3 specular = dirLightColor.rgb * materialSpecular * specFactor;

    // 3. Łączenie wszystkich składników
    vec3 result = ambient + diffuse + specular;

    // Ostateczny kolor fragmentu
    FragColor = vec4(result, 1.0);
}