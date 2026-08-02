#version 330 core

// Infinite procedural editor grid on the Y=0 plane. Each fragment casts a ray from the camera
// through the pixel, intersects it with the plane and shades anti-aliased grid lines — no
// geometry, the grid always covers the whole view. Engine scale: 1 world unit = 1 metre;
// minor lines every 1 m (kCellSize, fixed), major lines every 10 m, world axes tinted.
// Depth: gl_FragDepth is the plane point's real depth, so scene geometry occludes the grid
// (the pass itself runs with depth writes off — the grid never occludes anything).

in vec2 vNdcPos;
out vec4 FragColor;

uniform mat4 uViewProj;     // to compute gl_FragDepth of the plane point
uniform mat4 uInvViewProj;  // to reconstruct the per-pixel world ray
uniform vec3 uCameraPos;

const vec3  kMinorColor  = vec3(0.50, 0.50, 0.50);
const vec3  kMajorColor  = vec3(0.72, 0.72, 0.72);
const vec3  kAxisXColor  = vec3(0.90, 0.30, 0.32); // X axis = the z == 0 line
const vec3  kAxisZColor  = vec3(0.28, 0.48, 0.95); // Z axis = the x == 0 line
const float kCellSize    = 1.0;  // minor cell size in world units (1 m — engine scale)
const float kMajorEvery  = 10.0;
const float kMaxAlpha    = 0.6;
const float kFadeDistance = 250.0; // metres; keep below the far clip so the grid dissolves, not clips

// 1 at line centres, 0 in cell interiors; ~1 px wide, anti-aliased with screen-space derivatives.
float GridLine(vec2 coord)
{
    vec2 dist = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    return 1.0 - min(min(dist.x, dist.y), 1.0);
}

void main()
{
    // Unproject the pixel at the near and far plane to get the world-space view ray.
    vec4 nearPoint = uInvViewProj * vec4(vNdcPos, -1.0, 1.0);
    vec4 farPoint  = uInvViewProj * vec4(vNdcPos,  1.0, 1.0);
    vec3 rayStart = nearPoint.xyz / nearPoint.w;
    vec3 rayDelta = farPoint.xyz / farPoint.w - rayStart;

    // Intersect with Y=0. t outside (0,1] means the plane is not visible through this pixel
    // (behind the camera or beyond the far plane). Guard the horizontal-ray division.
    if (abs(rayDelta.y) < 1e-9) discard;
    float t = -rayStart.y / rayDelta.y;
    if (t <= 0.0 || t > 1.0) discard;
    vec3 world = rayStart + t * rayDelta;

    // Real depth of the plane point — lets the (already rendered) scene occlude the grid.
    vec4 clip = uViewProj * vec4(world, 1.0);
    gl_FragDepth = (clip.z / clip.w) * 0.5 + 0.5;

    vec2 cellCoord = world.xz / kCellSize;
    float minorLine = GridLine(cellCoord);
    float majorLine = GridLine(cellCoord / kMajorEvery);

    // Fade minor lines out when a cell shrinks below a few pixels (grazing angles, far
    // distance) — otherwise they alias into solid noise. Major lines take over there.
    vec2 cellPx = 1.0 / fwidth(cellCoord);
    float minorVisibility = clamp((min(cellPx.x, cellPx.y) - 2.0) / 6.0, 0.0, 1.0);

    // World axes drawn slightly thicker than grid lines, on top of everything.
    vec2 axisDist = abs(world.xz) / (fwidth(world.xz) * 1.5);
    float axisZLine = 1.0 - min(axisDist.x, 1.0); // x == 0
    float axisXLine = 1.0 - min(axisDist.y, 1.0); // z == 0

    // Strongest contribution wins: minor < major < axes.
    vec3 color = kMinorColor;
    float line = minorLine * minorVisibility;
    if (majorLine > line) { line = majorLine; color = kMajorColor; }
    if (axisXLine > line) { line = axisXLine; color = kAxisXColor; }
    if (axisZLine > line) { line = axisZLine; color = kAxisZColor; }

    // Radial fade so the grid dissolves with distance instead of ending abruptly.
    float fade = clamp(1.0 - length(world - uCameraPos) / kFadeDistance, 0.0, 1.0);

    float alpha = line * fade * kMaxAlpha;
    if (alpha <= 0.003) discard;
    FragColor = vec4(color, alpha);
}
