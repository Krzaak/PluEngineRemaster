#version 330 core

// Fullscreen triangle generated from gl_VertexID — no vertex buffer, the bound VAO is empty
// (Renderer::mGridVao). NDC positions (-1,-1), (3,-1), (-1,3) cover the whole screen.
out vec2 vNdcPos;

void main()
{
    vec2 pos = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2)) * 2.0 - 1.0;
    vNdcPos = pos;
    gl_Position = vec4(pos, 0.0, 1.0);
}
