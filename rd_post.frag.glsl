#version 330 core

uniform sampler2D u_texture;
uniform float     u_blur;        // texel-space offset scale (0 = sharp)
uniform float     u_brightness;  // multiplier; >1 overexposes toward white

in  vec2 v_uv;
out vec4 fragColor;

void main() {
    // 5x5 Gaussian blur (weights approximate sigma=1)
    float kernel[25] = float[25](
        1.0,  4.0,  7.0,  4.0, 1.0,
        4.0, 16.0, 26.0, 16.0, 4.0,
        7.0, 26.0, 41.0, 26.0, 7.0,
        4.0, 16.0, 26.0, 16.0, 4.0,
        1.0,  4.0,  7.0,  4.0, 1.0
    );
    float total = 273.0;

    vec4 color = vec4(0.0);
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            int  idx = (y + 2) * 5 + (x + 2);
            vec2 uv  = v_uv + vec2(float(x), float(y)) * u_blur;
            color   += texture(u_texture, uv) * kernel[idx];
        }
    }
    color /= total;

    fragColor = clamp(color * u_brightness, 0.0, 1.0);
}
