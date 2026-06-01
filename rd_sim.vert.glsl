#version 330 core

// Full-screen triangle: 3 dummy vertices, positions computed from gl_VertexID.
// Covers the entire NDC square [-1,1]x[-1,1] with correct UVs [0,1]x[0,1].

out vec2 v_uv;

void main() {
    const vec2 pos[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    const vec2 uvs[3] = vec2[3](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    v_uv        = uvs[gl_VertexID];
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}
