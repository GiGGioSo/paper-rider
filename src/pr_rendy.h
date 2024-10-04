#ifndef _RENDY_H_
#define _RENDY_H_

///
////////////////////////////////////
/// RY - RENDY THE (2D) RENDERER ///
////////////////////////////////////
///

#include "pr_mathy.h"

// TODO(gio):
// [x] Layer registration (creation)
// [x] Function to initialize the context
// [x] Target creation
// [ ] ArrayTexture creation
// [ ] Single function call to draw all the layers
// [ ] Tests!! please compile

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

/*
 * #######################
 * ### DATA STRUCTURES ###
 * #######################
 */

// ### Textures ###
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
// ################

typedef enum RY_Err {
    RY_ERR_NONE = 0;
    RY_ERR_LAYER_INDEX_OUT_OF_BOUNDS = 1;
    RY_ERR_INVALID_ARGUMENTS = 2;
    RY_ERR_NOT_IMPLEMENTED = 3;
    RY_ERR_MEMORY_ALLOCATION = 4;
    RY_ERR_OUT_OF_BUFFER_MEMORY = 5;
} RY_Err;

typedef enum RY_LayerFlags {
    RY_LAYER_TRANSPARENT = 1 << 0;
    RY_LAYER_TEXTURED = 1 << 1;
} RY_LayerFlags;

typedef struct RY_Target {
    GLuint vao;

    uint32 vertex_size; // number of bytes needed to store a single vertex

    GLuint vbo;
    uint32 vbo_bytes;
    uint32 vbo_capacity;

    GLuint ebo;
    uint32 ebo_bytes;
    uint32 ebo_capacity;
} RY_Target;


typedef struct RY_Stats {
    uint64 draw_calls = 0;
} RY_Stats;

typedef struct RY_Rendy {
    RY_Layers layers;

    RY_Error err;
} RY_Rendy;

typedef struct RY_Layers {
    RY_Layer *elements;
    uint32 count; // number of elements present
    uint32 size;  // buffer capacity in number of elements
} RY_Layers;

typedef struct RY_Layer {
    uint32 sort_key;
    uint32 flags;

    RY_ArrayTexture array_texture;

    RY_Target target;
    RY_ShaderProgram program;

    RY_DrawCommands draw_commands;
    RY_IndexBuffer index_buffer;
    RY_VertexBuffer vertex_buffer;
} RY_Layer;

typedef struct RY_VertexBuffer {
    void *vertices_data;
    uint32 vertices_bytes;
    uint32 buffer_bytes;
} RY_VertexBuffer;

typedef struct RY_IndexBuffer {
    void *indices_data;
    uint32 indices_bytes;
    uint32 buffer_bytes;
} RY_IndexBuffer;

typedef struct RY_DrawCommands {
    RY_DrawCommand *elements;
    uint32 count; // number of elements present
    uint32 size;  // buffer capacity in number of elements
} RY_DrawCommands;

typedef struct RY_DrawCommand {
    uint64 sort_key; // intra-level sorting

    void *vertices_data_start;
    uint32 vertices_data_bytes;

    void *indices_data_start;
    uint32 indices_data_bytes;
} RY_DrawCommand;

/*
 * #################
 * ### FUNCTIONS ###
 * #################
 */

// ### API functions ###

RY_Rendy *
ry_init();

RY_Target
ry_create_target(RY_Rendy *ry, uint32 *vertex_info, uint32 vertex_info_length uint32 max_vertices_number);

void
ry_register_layer(RY_Rendy *ry, uint32 sort_key, RY_ArrayTexture array_texture, RY_ShaderProgram program, RY_Target target, uint32 flags);

//  - check for the context error after each usage
//  - context error is set to RY_ERR_NONE at the end of each one of them

/*
 * vertices -> If NULL the context error is set to RY_ERR_INVALID_ARGUMENTS.
 *             The size of the vertex is determined by the target of the level.
 *  
 * indices -> If NULL a default triangulation takes place with the
 *             following assumptions:
 *              - the polygon is convex                        
 *              - the vertices are in counter clock-wise order
 */
void
ry_push_polygon(RY_Rendy *ry, uint32 layer_index, uint64 in_layer_sort, void *vertices, uint32 vertices_number, uint32 *indices, uint32 indices_number);

char *
ry_err_string(RY_Rendy *ry);

// ### Internal functions ###

void
ry__insert_draw_command(RY_DrawCommands *commands, RY_DrawCommand cmd);

void *
ry__push_vertex_data_to_buffer(RY_Rendy *ry, RY_VertexBuffer *vb, void *vertices, uint32 vertices_bytes);

void *
ry__push_index_data_to_buffer(RY_Rendy *ry, RY_IndexBuffer *ib, void *indices, uint32 indices_bytes);

/*
 * #######################
 * ### IMPLEMENTATIONS ###
 * #######################
 */
#ifdef RENDY_IMPLEMENTATION

// ### API functions ###

RY_Rendy *ry_init() {
    RY_Rendy *ry = malloc(sizeof(RY_Rendy));
    if (ry == NULL) return NULL;

    ry->err = RY_ERR_NONE;

    return ry;
}

RY_Target ry_create_target(
        RY_Rendy *ry,
        uint32 *vertex_info,
        uint32 vertex_info_length
        uint32 max_vertices_number) {
    RY_Target target = {};

    // vertex info should come in tris
    if (ry == NULL || vertex_info == NULL || vertex_info_length % 3 != 0) {
        ry->err = RY_ERR_INVALID_ARGUMENTS;
        return target;
    }

    // calculate vertex size in bytes
    uint32 vertex_size = 0;
    for(uint32 info_index = 0;
        info_index < vertex_info_length / 3;
        ++info_index) {
        uint32 type_bytes = vertex_info[info_index * 3 + 1];
        uint32 type_repetitions = vertex_info[info_index * 3 + 2];

        vertex_size += type_bytes * type_repetitions;
    }
    target.vertex_size = vertex_size;

    glGenVertexArrays(1, &target.vao);

    glGenBuffers(1, &target.vbo);
    target.vbo_bytes = 0;
    target.vbo_capacity = target.vertex_size * max_vertices_number;

    glGenBuffers(1, &target.ebo);
    target.ebo_bytes = 0;
    target.ebo_capacity = sizeof(uint32) * max_vertices_number;

    glBindVertexArray(target.vao);

    glBindBuffer(GL_ARRAY_BUFFER, target.vbo);
    // TODO(gio): you sure about GL_DYNAMIC_DRAW ?
    glBufferData(GL_ARRAY_BUFFER, target.vbo_capacity, NULL, GL_DYNAMIC_DRAW);

    // set vertex attributes
    uint32 current_info_offset = 0;
    for(uint32 vertex_info_index = 0;
        vertex_info_index < vertex_info_length / 3;
        ++vertex_info_index) {

        uint32 type_enum = vertex_info[vertex_info_index * 3 + 0];
        uint32 type_bytes = vertex_info[vertex_info_index * 3 + 1];
        uint32 type_repetitions = vertex_info[vertex_info_index * 3 + 2];

        glEnableVertexAttribArray(vertex_info_index);
        glVertexAttribPointer(
                vertex_info_index,
                type_repetitions,
                type_enum,
                GL_FALSE,
                target.vertex_size,
                (void *) current_info_offset);

        current_info_offset += type_bytes * type_repetitions;
    }

    ry->err = RY_ERR_NONE;
    return target;
}

RY_Layer *ry_register_layer(
        RY_Rendy *ry,
        uint32 sort_key,
        RY_ArrayTexture array_texture,
        RY_ShaderProgram program,
        RY_Target target,
        uint32 flags) {

    // allocate the memory
    RY_Layers *layers = &ry->layers;
    if (layers->count >= layers->size) {
        layers->size = (layers->size * 2) + 1;
        layers->elements = realloc(
                layers->elements,
                layers->size * sizeof(RY_Layer));
        if (!layers->elements) {
            ry->err = RY_ERR_MEMORY_ALLOCATION;
            return NULL;
        }
    }

    // find in order position
    uint32 comparison_index;
    for(comparison_index = 0;
        comparison_index < commands->count;
        ++comparison_index) {

        RY_Layer *comparison_layer = &layers->elements[comparison_index];
        if (sort_key < comparison_cmd->sort_key) {
            break;
        }
    }

    if (comparison_index < layers->count) { // move the data afterwards
        memmove(layers->elements + comparison_index+1,
                layers->elements + comparison_index,
                layers->count - comparison_index);
    }
    layers->count++;

    // fill in the layer
    RY_Layer *layer = &layers->elements[comparison_index];

    layer->sort_key = sort_key;
    layer->array_texture = array_texture;
    layer->program = program;
    layer->target = target;
    layer->flags = flags;
    layer->draw_commands = {};

    layer->index_buffer = {};
    layer->index_buffer.buffer_bytes = target.ebo_capacity;
    layer->index_buffer.indices_data =
        (uint32 *) malloc(layer->index_buffer.buffer_bytes);
    if (layer->index_buffer.indices_data == NULL) {
        ry->err = RY_ERR_MEMORY_ALLOCATION;
        return NULL;
    }

    layer->vertex_buffer = {};
    layer->vertex_buffer.buffer_bytes = target.vbo_capacity;
    layer->vertex_buffer.vertices_data =
        (uint32 *) malloc(layer->vertex_buffer.buffer_bytes);
    if (layer->vertex_buffer.vertices_data == NULL) {
        ry->err = RY_ERR_MEMORY_ALLOCATION;
        return NULL;
    }

    ry->err = RY_ERR_NONE;
    return layer;
}

void ry_push_polygon(
        RY_Rendy *ry,
        uint32 layer_index,
        uint64 in_layer_sort,
        void *vertices,
        uint32 vertices_number,
        uint32 *indices,
        uint32 indices_number) {

    if (vertices == NULL) {
        ry->err = RY_ERR_INVALID_ARGUMENTS;
        return;
    }

    if (layer_index >= ry->layers_count) {
        ry->err = RY_ERR_LAYER_INDEX_OUT_OF_BOUNDS;
        return;
    }

    RY_Layer *layer = &ry->layers[layer_index];
    RY_Target *target = &layer->target;

    if (indices == NULL) {
        // TODO(gio): Default triangulation/indicization, which assumes:
        //              - the polygon is convex
        //              - the vertices are in counter clock-wise order
        ry->err = RY_ERR_NOT_IMPLEMENTED;
        return;
    }

    // offset indices based on already present vertices
    uint32 index_offset =
        layer->vertex_buffer.vertices_bytes / target->vertex_size;
    for(uint32 index_index = 0;
        index_index < indices_number;
        ++index_index) {
        indices[index_index] += index_offset;
    }

    RY_DrawCommand cmd = {};

    cmd.sort_key = in_layer_sort;

    cmd.vertices_data_bytes = vertices_number * sizeof(*vertices);
    cmd.vertices_data_start =
        ry__push_vertex_data_to_buffer(
                &layer->vertex_buffer,
                (void *)vertices,
                cmd.vertices_data_bytes);
    if (ry->err) return;

    cmd.indices_data_bytes = indices_number * sizeof(*indices);
    cmd.indices_data_start =
        ry__push_index_data_to_buffer(
                &layer->index_buffer,
                (void *)indices,
                cmd.indices_data_bytes);
    if (ry->err) return;

    ry__insert_draw_command(ry, &layer->draw_commands, cmd);
    if (ry->err) return;

    ry->err = RY_ERR_NONE;
}

char *ry_err_string(RY_Rendy *ry) {
    if (ry->err == RY_ERR_NONE) {
        return "No error";
    } else if (ry->err == RY_ERR_LAYER_INDEX_OUT_OF_BOUNDS) {
        return "Layer index out of bounds";
    } else if (ry->err == RY_ERR_INVALID_ARGUMENTS) {
        return "Invalid function arguments";
    } else if (ry->err == RY_ERR_NOT_IMPLEMENTED) {
        return "Feature not implemented";
    } else if (ry->err == RY_ERR_MEMORY_ALLOCATION) {
        return "Failed during a memory allocation";
    }

    return "Unknown error";
}

// ### Internal functions ###

void ry__draw_layer(
        RY_Rendy *ry,
        RY_Layer *layer) {

    RY_DrawCommands *commands = &layer->draw_commands;
    RY_Target *target = &layer->target;

    for(uint32 cmd_index = 0;
        cmd_index < layer->draw_commands.count;
        ++cmd_index) {

        RY_DrawCommand *cmd = &commands->elements[cmd_index];

        glBindVertexArray(target->vao);

        // Setting the vertices data into the VBO
        glBindBuffer(GL_ARRAY_BUFFER, target->vbo);
        glBufferSubData(
                GL_ARRAY_BUFFER,
                target->vbo_bytes,
                cmd->vertices_data_bytes,
                cmd->vertices_data_start);
        target->vbo_bytes += cmd->vertices_data_bytes;
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Setting the indices data into the EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, target->ebo);
        glBufferSubData(
                GL_ELEMENT_ARRAY_BUFFER,
                target->ebo_bytes,
                cmd->indices_data_bytes,
                cmd->indices_data_start);
        target->ebo_bytes += cmd->vertices_data_bytes;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindVertexArray(0); // maybe you don't need to unbind it now
    }

    // TODO(gio): Do the actual draw call with glDrawElements(...)
    ry->err = RY_ERR_NOT_IMPLEMENTED;
    return;

    // glUseProgram(s);
    // shaderer_set_int(s, "tex", 0); // something like this
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, t->id);
    // glBindVertexArray(renderer->tex_vao);
    // glDrawArrays(GL_TRIANGLES, 0, renderer->tex_vertex_count);
    // glBindTexture(GL_TEXTURE_2D, 0);
    // glBindVertexArray(0);
    // target->vbo_bytes = 0;
    // target->ebo_bytes = 0;

    ry->err = RY_ERR_NONE;
    return;
}

void ry__insert_draw_command(
        RY_Rendy *ry,
        RY_DrawCommands *commands,
        RY_DrawCommand cmd) {

    if (commands->count >= commands->size) {
        commands->size = (commands->size * 2) + 1;
        commands->elements = realloc(
                commands->elements,
                commands->size * sizeof(RY_DrawCommand));
        if (!commands->elements) {
            ry->err = RY_ERR_MEMORY_ALLOCATION;
            return;
        }
    }

    // Inserting in order
    uint32 comparison_index;
    for(comparison_index = 0;
        comparison_index < commands->count;
        ++comparison_index) {

        RY_DrawCommand *comparison_cmd = &commands->elements[comparison_index];
        if (cmd.sort_key < comparison_cmd->sort_key) {
            break;
        }
    }

    if (comparison_index < commands->count) { // move the data afterwards
        memmove(commands->elements + comparison_index+1,
                commands->elements + comparison_index,
                commands->count - comparison_index);
    }

    commands->elements[comparison_index] = cmd;
    commands->count++;

    ry->err = RY_ERR_NONE;
    return;
}

void *ry__push_vertex_data_to_buffer(
        RY_Rendy *ry,
        RY_VertexBuffer *vb,
        void *vertices,
        uint32 vertices_bytes) {

    // The vertex buffer has the size of the target buffer
    if (vb->vertices_bytes + vertices_bytes >= vb->buffer_bytes) {
        ry->RY_ERR_OUT_OF_BUFFER_MEMORY;
        return;
    }

    memcpy(vb->vertices_data + vb->vertices_bytes, vertices, vertices_bytes);
    vb->vertices_bytes += vertices_bytes;

    ry->err = RY_ERR_NONE;
    return;
}

void *ry__push_index_data_to_buffer(
        RY_Rendy *ry,
        RY_IndexBuffer *ib,
        void *indices,
        uint32 indices_bytes) {

    // The index buffer has the size of the target buffer
    if (vb->indices_bytes + indices_bytes >= vb->buffer_bytes) {
        ry->RY_ERR_OUT_OF_BUFFER_MEMORY;
        return;
    }

    memcpy(vb->indices_data + vb->indices_bytes, indices, indices_bytes);
    vb->indices_bytes += indices_bytes;

    ry->err = RY_ERR_NONE;
    return;
}

#endif // RENDY_IMPLEMENTATION

#endif // _RENDY_H_
