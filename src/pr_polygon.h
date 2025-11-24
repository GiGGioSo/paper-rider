#ifndef _PR_POLYGON_H_ 
#define _PR_POLYGON_H_ 

#include "pr_mathy.h"

// s/\(\S\)69\(\s\)/\1##N\2/g
//  from 'Polygon69 add' to 'Polygon##N add'

// s/\(\S\)69\(\S\)/\1##N##\2/g
//  from 'poly69_add' to 'poly##n##_add'

// s/\(\S\)$/\1\\/g
//  add '\' to the end of each row


/*
 * For collisions: https://github.com/GiGGioSo/gjk2D
 */

typedef struct Edge {
    union {
        uint32 e[2];
        struct {
            uint32 v1, v2;
        };
    };
} Edge;

#define POLYGON__DEF(N) typedef struct Polygon##N {\
    vec2f vertices[N];\
    uint32 n_vertices;\
} Polygon##N;

// Polygon42 *poly42_add_vertex(Polygon42 *poly, Edge edge);
#define poly_add_vertex__def(N)\
Polygon##N *poly##N##_add_vertex(Polygon##N *poly, Edge edge);
#define poly_add_vertex__impl(N)\
Polygon##N *poly##N##_add_vertex(Polygon##N *poly, Edge edge) {\
    if (poly->n_vertices >= N) { return NULL; }\
    vec2f v1 = poly->vertices[edge.v1];\
    vec2f v2 = poly->vertices[edge.v2];\
    vec2f new_pos = (vec2f) { .x = (v1.x+v2.x)/2.f, .y = (v1.y+v2.y)/2.f };\
    poly##N##_move_vertices_forward_from(poly, edge.v2);\
    poly->vertices[edge.v2] = new_pos;\
    return poly;\
}

// Polygon42 *poly42_move_vertices_forward_from(Polygon42 *poly, uint32 from);
#define poly_move_vertices_forward__def(N)\
Polygon##N *poly##N##_move_vertices_forward_from(Polygon##N *poly, uint32 from);
#define poly_move_vertices_forward__impl(N)\
Polygon##N *poly##N##_move_vertices_forward_from(Polygon##N *poly, uint32 from) {\
    for(uint32 move_to = poly->n_vertices; move_to > from; move_to--) {\
        uint32 move_from = move_to - 1;\
        poly->vertices[move_to] = poly->vertices[move_from];\
    }\
    return poly;\
}

#define GENERATE_POLYGON_DEFINITIONS(N)\
    POLYGON__DEF(N)\
    poly_add_vertex__def(N)\
    poly_move_vertices_forward__def(N)

#ifdef PR_POLYGON_IMPLEMENTATION
#define GENERATE_POLYGON_IMPLEMENTATIONS(N)\
    poly_add_vertex__impl(N)\
    poly_move_vertices_forward__impl(N)
#else
#define GENERATE_POLYGON_IMPLEMENTATIONS(N)
#endif

#define GENERATE_POLYGON(N)\
    GENERATE_POLYGON_DEFINITIONS(N)\
    GENERATE_POLYGON_IMPLEMENTATIONS(N)

GENERATE_POLYGON(3)

#endif//_PR_POLYGON_H_
