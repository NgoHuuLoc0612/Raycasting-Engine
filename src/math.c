#include "../include/engine.h"
#include <math.h>

#define PI 3.14159265359f

// ===== Vec2 Operations =====
Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

Vec2 vec2_mul(Vec2 v, float scalar) {
    return (Vec2){v.x * scalar, v.y * scalar};
}

float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float vec2_length(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_length(v);
    if (len > 0.0f) {
        return vec2_mul(v, 1.0f / len);
    }
    return v;
}

Vec2 vec2_rotate(Vec2 v, float angle) {
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    return (Vec2){
        v.x * cos_a - v.y * sin_a,
        v.x * sin_a + v.y * cos_a
    };
}

// ===== Vec3 Operations =====
Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 vec3_mul(Vec3 v, float scalar) {
    return (Vec3){v.x * scalar, v.y * scalar, v.z * scalar};
}

float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len > 0.0f) {
        return vec3_mul(v, 1.0f / len);
    }
    return v;
}

// ===== Mat4 Operations =====
Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

Mat4 mat4_perspective(float fov, float aspect, float near, float far) {
    Mat4 m = {0};
    float tan_half_fov = tanf(fov * 0.5f * PI / 180.0f);
    
    m.m[0][0] = 1.0f / (aspect * tan_half_fov);
    m.m[1][1] = 1.0f / tan_half_fov;
    m.m[2][2] = -(far + near) / (far - near);
    m.m[2][3] = -1.0f;
    m.m[3][2] = -(2.0f * far * near) / (far - near);
    
    return m;
}

Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);
    
    Mat4 m = mat4_identity();
    m.m[0][0] = s.x;
    m.m[1][0] = s.y;
    m.m[2][0] = s.z;
    m.m[0][1] = u.x;
    m.m[1][1] = u.y;
    m.m[2][1] = u.z;
    m.m[0][2] = -f.x;
    m.m[1][2] = -f.y;
    m.m[2][2] = -f.z;
    m.m[3][0] = -vec3_dot(s, eye);
    m.m[3][1] = -vec3_dot(u, eye);
    m.m[3][2] = vec3_dot(f, eye);
    
    return m;
}

Mat4 mat4_multiply(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    
    return result;
}

Vec3 mat4_transform_vec3(Mat4 m, Vec3 v) {
    Vec3 result;
    float w = m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3];
    
    result.x = (m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0]) / w;
    result.y = (m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1]) / w;
    result.z = (m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2]) / w;
    
    return result;
}

// ===== Quaternion Operations =====
Quaternion quat_from_euler(float pitch, float yaw, float roll) {
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);
    
    Quaternion q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    
    return q;
}

Quaternion quat_multiply(Quaternion a, Quaternion b) {
    Quaternion result;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return result;
}

Vec3 quat_rotate_vec3(Quaternion q, Vec3 v) {
    Vec3 u = {q.x, q.y, q.z};
    float s = q.w;
    
    Vec3 vprime = vec3_add(
        vec3_add(vec3_mul(u, 2.0f * vec3_dot(u, v)),
                vec3_mul(v, s * s - vec3_dot(u, u))),
        vec3_mul(vec3_cross(u, v), 2.0f * s)
    );
    
    return vprime;
}

// ===== Fast Math Approximations =====
float fast_inv_sqrt(float x) {
    union {
        float f;
        uint32_t i;
    } conv = {.f = x};
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f *= 1.5f - (x * 0.5f * conv.f * conv.f);
    return conv.f;
}

float fast_sqrt(float x) {
    return x * fast_inv_sqrt(x);
}

float fast_sin(float x) {
    // Bhaskara I approximation
    x = fmodf(x, 2.0f * PI);
    if (x < 0) x += 2.0f * PI;
    
    if (x < PI) {
        float x2 = x * (PI - x);
        return 16.0f * x2 / (5.0f * PI * PI - 4.0f * x2);
    } else {
        x -= PI;
        float x2 = x * (PI - x);
        return -16.0f * x2 / (5.0f * PI * PI - 4.0f * x2);
    }
}

float fast_cos(float x) {
    return fast_sin(x + PI * 0.5f);
}

float fast_atan2(float y, float x) {
    float abs_y = fabsf(y) + 1e-10f;
    float r, angle;
    
    if (x >= 0) {
        r = (x - abs_y) / (x + abs_y);
        angle = PI * 0.25f - PI * 0.25f * r;
    } else {
        r = (x + abs_y) / (abs_y - x);
        angle = PI * 0.75f - PI * 0.25f * r;
    }
    
    return y < 0 ? -angle : angle;
}

// ===== Noise Generation =====
static int noise_perm[512];
static bool noise_initialized = false;

static void init_noise(void) {
    if (noise_initialized) return;
    
    int p[256] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
    
    for (int i = 0; i < 256; i++) {
        noise_perm[i] = noise_perm[256 + i] = p[i];
    }
    
    noise_initialized = true;
}

static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

static float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlin_noise_3d(float x, float y, float z) {
    init_noise();
    
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;
    int Z = (int)floorf(z) & 255;
    
    x -= floorf(x);
    y -= floorf(y);
    z -= floorf(z);
    
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);
    
    int A = noise_perm[X] + Y;
    int AA = noise_perm[A] + Z;
    int AB = noise_perm[A + 1] + Z;
    int B = noise_perm[X + 1] + Y;
    int BA = noise_perm[B] + Z;
    int BB = noise_perm[B + 1] + Z;
    
    return lerp(w, 
        lerp(v, 
            lerp(u, grad(noise_perm[AA], x, y, z),
                   grad(noise_perm[BA], x - 1, y, z)),
            lerp(u, grad(noise_perm[AB], x, y - 1, z),
                   grad(noise_perm[BB], x - 1, y - 1, z))),
        lerp(v,
            lerp(u, grad(noise_perm[AA + 1], x, y, z - 1),
                   grad(noise_perm[BA + 1], x - 1, y, z - 1)),
            lerp(u, grad(noise_perm[AB + 1], x, y - 1, z - 1),
                   grad(noise_perm[BB + 1], x - 1, y - 1, z - 1))));
}

float perlin_noise_2d(float x, float y) {
    return perlin_noise_3d(x, y, 0.0f);
}

float simplex_noise_2d(float x, float y) {
    init_noise();
    
    const float F2 = 0.5f * (sqrtf(3.0f) - 1.0f);
    const float G2 = (3.0f - sqrtf(3.0f)) / 6.0f;
    
    float s = (x + y) * F2;
    int i = (int)floorf(x + s);
    int j = (int)floorf(y + s);
    
    float t = (i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    
    int i1 = x0 > y0 ? 1 : 0;
    int j1 = x0 > y0 ? 0 : 1;
    
    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;
    
    int ii = i & 255;
    int jj = j & 255;
    
    int gi0 = noise_perm[ii + noise_perm[jj]] % 12;
    int gi1 = noise_perm[ii + i1 + noise_perm[jj + j1]] % 12;
    int gi2 = noise_perm[ii + 1 + noise_perm[jj + 1]] % 12;
    
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    float n0 = t0 < 0 ? 0.0f : powf(t0, 4) * grad(gi0, x0, y0, 0);
    
    float t1 = 0.5f - x1 * x1 - y1 * y1;
    float n1 = t1 < 0 ? 0.0f : powf(t1, 4) * grad(gi1, x1, y1, 0);
    
    float t2 = 0.5f - x2 * x2 - y2 * y2;
    float n2 = t2 < 0 ? 0.0f : powf(t2, 4) * grad(gi2, x2, y2, 0);
    
    return 70.0f * (n0 + n1 + n2);
}
