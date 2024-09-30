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

typedef struct RY_TexCoords {
    union {
        struct {
            float tx, ty, tw, th;
        };
        float e[4];
    };
} RY_TexCoords;

typedef struct RY_TextureElement {
    char filename[256];
    int32 width;
    int32 height;
    RY_TexCoords tex_coords;
} RY_TextureElement;

typedef struct RY_ArrayTexture {
    RY_TextureElement *elements;
    int32 elements_len;
    uint32 id;
} RY_ArrayTexture;

typedef struct RY_Target {
    uint32 vao;

    uint32 vbo;
    uint32 vbo_bytes_offset;
    uint32 vbo_vertex_count;

    uint32 ebo;
    uint32 ebo_bytes_offset;
    uint32 ebo_index_count;
} RY_Target;


typedef struct RY_Stats {
    uint64 draw_calls = 0;
} RY_Stats;

typedef struct RY_Rendy {
    RY_Layer *layers;
    uint32 layers_count;

    RY_Error err;
} RY_Rendy;

typedef struct RY_Layer {
    bool transpacency = false; // force disable transparency
                               // decides render order (back->front or reverse)
    bool textured;
    ArrayTexture *array_texture;

    RY_Target target;

    RY_DrawCommands commands;
} RY_Layer;

typedef struct RY_DrawCommands {
    RY_DrawCommand *elements;
    uint32 count; // number of elements present
    uint32 size;  // buffer capacity in number of elements
} RY_DrawCommands;

typedef struct RY_DrawCommand {
    uint64 sort_key; // intra-level sorting

    void *vertices_data;
    uint32 vertices_data_length;

    void *indices_data;
    uint32 indices_data_length;
} RY_DrawCommand;

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
