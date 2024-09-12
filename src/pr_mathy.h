#ifndef _MATHY_H_
#define _MATHY_H_

///
////////////////////////////////////
/// TY - MATHY THE MATHEMATICIAN ///
////////////////////////////////////
///

struct vec2f {
    union {
        float e[2];
        struct {
            float x, y;
        };
    };
};

struct vec3f {
    union {
        float e[3];
        struct {
            float x, y, z;
        };
        struct {
            float r, g, b;
        };
    };
};

struct vec4f {
    union {
        float e[4];
        struct {
            float x, y, z, w;
        };
        struct {
            float r, g, b, a;
        };
    };
};

#endif //_MATHY_H_
