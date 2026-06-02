#version 330 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 texcoord;

out vec2 v_uv;

void main() {
    v_uv        = texcoord;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
