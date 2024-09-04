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

#endif //_MATHY_H_
