#version 330 core
// rd5_display.frag.glsl
// Flat color based on RD concentrations. No lighting.

in vec2 v_uv;

out vec4 fragColor;

uniform sampler2D u_texture;  // RD state: R=A, G=B
uniform vec3      u_colorA;   // color for high-A regions
uniform vec3      u_colorB;   // color for high-B regions
uniform vec3      u_colorBg;  // background / equilibrium color

void main() {
    vec2  st   = texture(u_texture, v_uv).rg;
    float a    = st.r;
    float b    = st.g;
    float diff = a - b;

    const float threshold = 0.1;
    vec3 color;
    if (diff > threshold) {
        float t = smoothstep(threshold, 1.0, diff);
        color = mix(u_colorBg, u_colorA, t);
    } else if (diff < -threshold) {
        float t = smoothstep(-threshold, -1.0, diff);
        color = mix(u_colorBg, u_colorB, t);
    } else {
        color = u_colorBg;
    }

    fragColor = vec4(color, 1.0);
}
