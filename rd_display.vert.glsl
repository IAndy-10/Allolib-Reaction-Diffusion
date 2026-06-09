#version 330 core
// rd5_display.vert.glsl
// Spherical tessellation with equirectangular UV.
// Seamless and continuous — no face seams. Slight polar distortion is
// acceptable from inside the Allosphere (audience looks forward, not straight up).
// SPHERE_LAT and SPHERE_LON must match the constants in 1.cpp.

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;
uniform sampler2D u_texture;   // RD state: R=A, G=B
uniform float     u_dispScale; // inward radial displacement scale
uniform float u_eyeSep;
uniform float u_focLen;

const int   SPHERE_LAT = 300;
const int   SPHERE_LON = 600;
const float PI         = 3.14159265358979323846;
const float RADIUS     = 60.0;

out vec2 v_uv;

vec4 stereo_displace(vec4 v, float e, float f) {
  // eye to vertex distance
  float l = sqrt((v.x - e) * (v.x - e) + v.y * v.y + v.z * v.z);
  // absolute z-direction distance
  float z = abs(v.z);
  // x coord of projection of vertex on focal plane when looked from eye
  float t = f * (v.x - e) / z;
  // x coord of displaced vertex to make displaced vertex be projected on focal plane
  // when looked from origin at the same point original vertex would be projected
  // when looked form eye
  v.x = z * (e + t) / f;
  // set distance from origin to displaced vertex same as eye to original vertex
  v.xyz = normalize(v.xyz);
  v.xyz *= l;
  return v;
}

void main() {
    int quadId  = gl_VertexID / 6;
    int subVert = gl_VertexID - quadId * 6;

    int latIdx = quadId / SPHERE_LON;
    int lonIdx = quadId % SPHERE_LON;

    bool higherLat = (subVert == 1 || subVert == 3 || subVert == 4);
    bool higherLon = (subVert == 2 || subVert == 4 || subVert == 5);

    int li = latIdx + (higherLat ? 1 : 0);
    int lo = lonIdx + (higherLon ? 1 : 0);

    float u = float(lo) / float(SPHERE_LON);
    float v = float(li) / float(SPHERE_LAT);
    v_uv = vec2(u, v);

    float phi   = u * 2.0 * PI;
    float theta = v * PI;

    vec3 dir = vec3(sin(theta) * cos(phi),
                    cos(theta),
                    sin(theta) * sin(phi));

    float b = texture(u_texture, v_uv).g;
    float r = RADIUS - b * u_dispScale;

    vec3 worldPos = dir * r;
    vec4 pos =  al_ModelViewMatrix * vec4(worldPos, 1.0);
    gl_Position = al_ProjectionMatrix * stereo_displace(pos, u_eyeSep, u_focLen);
}
