#version 330 core

// Full-screen triangle using AlloLib's standard attribute layout:
//   location 0 = position (vec3)
//   location 2 = texcoord (vec2)

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texcoord;

out vec2 v_uv;

void main() {
    v_uv        = texcoord;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
