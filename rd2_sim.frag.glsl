#version 330 core
// Gray-Scott reaction-diffusion step using a standard 3x3 Laplacian.
// Compatible with older GPUs (9 texture samples vs 81 in bi-Laplacian).

in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_texture;   // current state: R=A, G=B
uniform vec2      u_resolution;
uniform float     u_dA;
uniform float     u_dB;
uniform float     u_feed;
uniform float     u_k;
uniform float     u_dt;

// Standard 3x3 Laplacian (weights: center=-1, edges=0.2, corners=0.05)
vec2 laplace(vec2 uv) {
    vec2 t = 1.0 / u_resolution;
    vec2 sum = texture(u_texture, uv).rg * -1.0;
    sum += texture(u_texture, uv + vec2(-t.x,  0.0)).rg * 0.2;
    sum += texture(u_texture, uv + vec2( t.x,  0.0)).rg * 0.2;
    sum += texture(u_texture, uv + vec2( 0.0, -t.y)).rg * 0.2;
    sum += texture(u_texture, uv + vec2( 0.0,  t.y)).rg * 0.2;
    sum += texture(u_texture, uv + vec2(-t.x, -t.y)).rg * 0.05;
    sum += texture(u_texture, uv + vec2( t.x, -t.y)).rg * 0.05;
    sum += texture(u_texture, uv + vec2( t.x,  t.y)).rg * 0.05;
    sum += texture(u_texture, uv + vec2(-t.x,  t.y)).rg * 0.05;
    return sum;
}

void main() {
    vec4  current  = texture(u_texture, v_uv);
    float a        = current.r;
    float b        = current.g;

    vec2  lap      = laplace(v_uv);
    float reaction = a * b * b;

    // Gray-Scott equations with standard Laplacian diffusion
    float nextA = a + u_dt * (u_dA * lap.r - reaction + u_feed * (1.0 - a));
    float nextB = b + u_dt * (u_dB * lap.g + reaction - (u_k + u_feed) * b);

    fragColor = vec4(clamp(nextA, 0.0, 1.0), clamp(nextB, 0.0, 1.0), 0.0, 1.0);
}
