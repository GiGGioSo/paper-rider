#ifndef _MATHY_H_
#define _MATHY_H_

///
////////////////////////////////////
/// TY - MATHY THE MATHEMATICIAN ///
////////////////////////////////////
///

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

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

vec4f mat4f_x_vec4f(mat4f m, vec4f v) {
    vec4f result = {};

    for(int i = 0; i < 4; i++) {
        float row_x_column = 0.f;
        for(int j = 0; j < 4; j++) {
            row_x_column += m.m[i][j] * vec.e[j];
        }
        result.e[i] = row_x_column;
    }

    return result;
}

// x x x x    x x x x     x x x x
//                |  
// x_x_x_x    x x x x     x x o x
//                |  
// x x x x    x x x x     x x x x
//                |  
// x x x x    x x x x     x x x x

mat4f mat4f_x_mat4f(mat4f m1, mat4f m2) {
    mat4f result = {};

    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {

            float row_x_column = 0.f;
            for(int iter = 0; iter < 4; iter++) {
                row_x_column += m1.m[row][iter] * m2.m[iter][col];
            }
            result.m[i][j] = row_x_column;
        }
    }

    return result;
}

#endif //_MATHY_H_
