#ifndef _RENDY_H_
#define _RENDY_H_

///
////////////////////////////////////
/// RY - RENDY THE (2D) RENDERER ///
////////////////////////////////////
///

#include "pr_mathy.h"

// Custom z layering

// Layers are just stuff drawn together.
// Inside of the layers we have different orderings based on additional z data

// - group by z level (needs a stable sorting)
// - group by shader
// - group by opaque/transparent
//      (maybe nothing is transparent? because
//          everything is a piece of paper,
//          should I still implement the possibility?)

//  I will have an effect like things are sticked to the screen as they appear:
//    - each layer itself must be rendered from left to right
//   _OR_
//    - i could calculate the left most vertex of a polygon in the
//       rendering pipeline? maybe inside the geometry/vertex shader?
//   _OR_
//    - the client does this by setting an order value inside of the layer, by which stuff is sorted

// TODO(gio): Layers based VS Objects based:

#define MAX_POLYGON_VERTICES 6

typedef enum RY_Err {
    RY_ERR_NONE = 0,
    RY_ERR_LAYER_INDEX_OUT_OF_BOUNDS = 1,
} RY_Err;

typedef struct RY_Vertex {
    vec2f pos;
} RY_Vertex;

typedef struct RY_Target {
    unsigned int vao;
    unsigned int vbo;
    unsigned int bytes_offset;
    unsigned int vertex_count;
} RY_Target;

typedef struct RY_Layer {
    bool transpacency = false; // force disable transparency
                               // decides render order (back->front or reverse)
    bool textured;
    ArrayTexture *array_texture;

    RY_Target target;
} RY_Layer;

typedef struct RY_Stats {
    int draw_calls = 0;
} RY_Stats;

typedef struct RY_Rendy {
    RY_Layer *layers;
    size_t layers_count;

    RY_Error err;
} RY_Rendy;

void ry_push_triangle() {
}

void ry_push_polygon(
        RY_Rendy *ry,
        int layer_index,
        float z,
        RY_Vertex vertices,
        int vertices_length) {

    if (layer_index >= ry->layers_count) {
        ry->err = RY_ERR_LAYER_INDEX_OUT_OF_BOUNDS;
        return;
    }

    // TODO: implement somehow
}

char *ry_err_string(RY_Rendy *ry) {
    if (ry->err == RY_ERR_NONE) {
        return "No error";
    } else if (ry->err == RY_ERR_LAYER_INDEX_OUT_OF_BOUNDS) {
        return "Layer index out of bounds";
    }

    return "Unknown error";
}

#endif // _RENDY_H_
