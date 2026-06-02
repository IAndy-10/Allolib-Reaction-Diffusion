#version 330 core
// rd_display.frag.glsl
// Flat color based on RD concentrations, with optional blur + brightness for stage four.

in vec2 v_uv;

out vec4 fragColor;

uniform sampler2D u_texture;   // RD state: R=A, G=B
uniform vec3      u_colorA;    // color for high-A regions
uniform vec3      u_colorB;    // color for high-B regions
uniform vec3      u_colorBg;   // background / equilibrium color
uniform float     u_dispScale; // passed through (used in vert shader)
uniform float     u_blur;      // texel-offset scale for blur (0 = sharp)
uniform float     u_brightness;// overexposure multiplier (1 = normal)
uniform float     u_whiteFade; // 0=normal, 1=pure white (last 3s of stage four)

vec3 rdColor(vec2 uv) {
    vec2  st   = texture(u_texture, uv).rg;
    float a    = st.r;
    float b    = st.g;
    float diff = a - b;

    const float threshold = 0.1;
    if (diff > threshold) {
        float t = smoothstep(threshold, 1.0, diff);
        return mix(u_colorBg, u_colorA, t);
    } else if (diff < -threshold) {
        float t = smoothstep(-threshold, -1.0, diff);
        return mix(u_colorBg, u_colorB, t);
    }
    return u_colorBg;
}

void main() {
    vec3 color;

    if (u_blur > 0.0) {
        // 5x5 Gaussian blur in UV space
        float kernel[25] = float[25](
            1.0,  4.0,  7.0,  4.0, 1.0,
            4.0, 16.0, 26.0, 16.0, 4.0,
            7.0, 26.0, 41.0, 26.0, 7.0,
            4.0, 16.0, 26.0, 16.0, 4.0,
            1.0,  4.0,  7.0,  4.0, 1.0
        );
        float total = 273.0;
        vec3 acc = vec3(0.0);
        for (int y = -2; y <= 2; y++) {
            for (int x = -2; x <= 2; x++) {
                int  idx = (y + 2) * 5 + (x + 2);
                vec2 uv  = v_uv + vec2(float(x), float(y)) * u_blur;
                acc += rdColor(uv) * kernel[idx];
            }
        }
        color = acc / total;
    } else {
        color = rdColor(v_uv);
    }

    vec3 bright = clamp(color * u_brightness, 0.0, 1.0);
    fragColor = vec4(mix(bright, vec3(1.0), u_whiteFade), 1.0);
}
