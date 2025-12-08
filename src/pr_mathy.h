#ifndef _PR_MATHY_H_
#define _PR_MATHY_H_

///
////////////////////////////////////
/// TY - MATHY THE MATHEMATICIAN ///
////////////////////////////////////
///

#include <stddef.h>
#include <stdint.h>
#include <math.h>

#ifndef ARR_LEN
#define ARR_LEN(x) ((int)(sizeof((x)) / sizeof((x)[0])))
#endif
#ifndef DIRECTION
#define DIRECTION(x) (((x) > 0) ? 1 : -1)
#endif
#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : (-(x)))
#endif
#ifndef SIGN
#define SIGN(x) (((x) > 0) ? 1 : (((x) < 0) ? -1 : 0))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif
#ifndef CLAMP
#define CLAMP(x, min, max) MIN(MAX((x), (min)), (max))
#endif

#ifndef POW2
#define POW2(x) ((x) * (x))
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 0xff
#endif

#ifndef UINT16_MAX
#define UINT16_MAX 0xffff
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffff
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef struct vec2i {
    union {
        int32 e[2];
        struct {
            int32 x, y;
        };
    };
} vec2i;

typedef struct vec3i {
    union {
        int32 e[3];
        struct {
            int32 x, y, z;
        };
        struct {
            int32 r, g, b;
        };
    };
} vec3i;

typedef struct vec4i {
    union {
        int32 e[4];
        struct {
            int32 x, y, z, w;
        };
        struct {
            int32 r, g, b, a;
        };
    };
} vec4i;

typedef struct vec2f {
    union {
        float e[2];
        struct {
            float x, y;
        };
    };
} vec2f;

typedef struct vec3f {
    union {
        float e[3];
        struct {
            float x, y, z;
        };
        struct {
            float r, g, b;
        };
    };
} vec3f;

typedef struct vec4f {
    union {
        float e[4];
        struct {
            float x, y, z, w;
        };
        struct {
            float r, g, b, a;
        };
    };
} vec4f;

typedef struct mat4f {
    union {
        float e[16];
        float m[4][4];
    };
} mat4f;

double
radians(double x);

float
radiansf(float x);

vec4f
_vec4f(float x, float y, float z, float w);

vec4f
_diag_vec4f(float x);

vec4f
vec4f_divide(vec4f v, float x);

vec4f
vec4f_mult(vec4f v, float x);

vec4f
vec4f_sum(vec4f v1, vec4f v2);

vec4f
vec4f_diff(vec4f v1, vec4f v2);

vec2f
_vec2f(float x, float y);

vec2f
_diag_vec2f(float v);

vec2f
vec2f_divide(vec2f v, float x);

vec2f
vec2f_mult(vec2f v, float x);

vec2f
vec2f_sum(vec2f v1, vec2f v2);

vec2f
vec2f_diff(vec2f v1, vec2f v2);

vec2f
vec2f_from_angle(float rad);

float
vec2f_len_sq(vec2f v);

float
vec2f_len(vec2f v);

vec2f
vec2f_normalize(vec2f v);

mat4f
orthographic(float left, float right, float bottom, float top, float near, float far);

vec4f
mat4f_x_vec4f(mat4f m, vec4f v);

mat4f
mat4f_x_mat4f(mat4f m1, mat4f m2);


#ifdef PR_MATHY_IMPLEMENTATION

double radians(double x) {
    return x * PI / 180.0;
}

float radiansf(float x) {
    return x * PI / 180.f;
}

vec4f _vec4f(float x, float y, float z, float w) {
    vec4f result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

vec4f _diag_vec4f(float v) {
    vec4f result;

    result.x = v;
    result.y = v;
    result.z = v;
    result.w = v;

    return result;
}

vec4f vec4f_divide(vec4f v, float x) {
    vec4f result;

    result.x = v.x / x;
    result.y = v.y / x;
    result.z = v.z / x;
    result.w = v.w / x;

    return result;
}

vec4f vec4f_mult(vec4f v, float x) {
    vec4f result;

    result.x = v.x * x;
    result.y = v.y * x;
    result.z = v.z * x;
    result.w = v.w * x;

    return result;
}

vec4f vec4f_sum(vec4f v1, vec4f v2) {
    vec4f result;

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    result.w = v1.w + v2.w;

    return result;
}

vec4f vec4f_diff(vec4f v1, vec4f v2) {
    vec4f result;

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    result.w = v1.w - v2.w;

    return result;
}

vec2f _vec2f(float x, float y) {
    vec2f result;

    result.x = x;
    result.y = y;

    return result;
}

vec2f _diag_vec2f(float v) {
    vec2f result;

    result.x = v;
    result.y = v;

    return result;
}

vec2f vec2f_divide(vec2f v, float x) {
    vec2f result;

    result.x = v.x / x;
    result.y = v.y / x;

    return result;
}

vec2f vec2f_mult(vec2f v, float x) {
    vec2f result;

    result.x = v.x * x;
    result.y = v.y * x;

    return result;
}

vec2f vec2f_sum(vec2f v1, vec2f v2) {
    vec2f result;

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;

    return result;
}

vec2f vec2f_diff(vec2f v1, vec2f v2) {
    vec2f result;

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;

    return result;
}


vec2f vec2f_from_angle(float rad) {
    vec2f result;

    result.x = cosf(rad);
    result.y = sinf(rad);

    return result;
}

int vec4f_equals(vec4f v1, vec4f v2) {
    return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w);
}

float vec2f_len_sq(vec2f v) {
    return v.x * v.x + v.y * v.y;
}

float vec2f_len(vec2f v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

vec2f vec2f_normalize(vec2f v) {
    vec2f result;

    float magnitude = vec2f_len(v);
    result.x = v.x / magnitude;
    result.y = v.y / magnitude;

    return result;
}

mat4f orthographic(float left, float right, float bottom, float top, float near, float far) {
    mat4f result = {
        .m = {
            { 2.f/(right-left), 0, 0, -(right+left)/(right-left)},
            { 0, 2.f/(top-bottom), 0, -(top+bottom)/(top-bottom)},
            { 0, 0, -2.f/(near-far),  -(far+near)/(far-near)},
            { 0, 0, 0, 1},
        }
    };

    return result;
}

vec4f mat4f_x_vec4f(mat4f m, vec4f v) {
    vec4f result = {};

    for(int i = 0; i < 4; i++) {
        float row_x_column = 0.f;
        for(int j = 0; j < 4; j++) {
            row_x_column += m.m[i][j] * v.e[j];
        }
        result.e[i] = row_x_column;
    }

    return result;
}

mat4f mat4f_x_mat4f(mat4f m1, mat4f m2) {
    mat4f result = {};

    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {

            float row_x_column = 0.f;
            for(int iter = 0; iter < 4; iter++) {
                row_x_column += m1.m[row][iter] * m2.m[iter][col];
            }
            result.m[row][col] = row_x_column;
        }
    }

    return result;
}

#endif //PR_MATHY_IMPLEMENTATION

#endif //_PR_MATHY_H_
