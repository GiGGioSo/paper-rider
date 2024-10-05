#ifndef _RENDY_H_
#define _RENDY_H_

///
////////////////////////////////////
/// RY - RENDY THE (2D) RENDERER ///
////////////////////////////////////
///

#include "pr_mathy.h"
#include "../include/stb_image.h"

// TODO(gio):
// [x] Layer registration (creation)
// [x] Function to initialize the context
// [x] Target creation
// [/] Shaders - defer deallocation of code buffers and closing of code files
// [/] ArrayTexture creation - improve creation
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

// ### SHADERS ###
typedef uint32 RY_ShaderProgram;
// ###############

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
ry_create_array_texture(RY_Rendy *ry, RY_ArrayTexture *at);

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

// # shaders
RY_ShaderProgram
ry_shader_create_program(RY_Rendy *ry, const char* vertexPath, const char* fragmentPath);
void
ry_shader_set_int32(RY_ShaderProgram s, const char* name, int32 value);
void
ry_shader_set_float(RY_ShaderProgram s, const char* name, float value);
void
ry_shader_set_mat4f(RY_ShaderProgram s, const char* name, mat4f value);
void
ry_shader_set_vec3f(RY_ShaderProgram s, const char* name, vec3f value);

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

void ry_create_array_texture(
        RY_Rendy *ry,
        RY_ArrayTexture *at) {
    stbi_set_flip_vertically_on_load(true);

    RY_DataImage empty_data_image = {};
    RY_DataImages images = {};

    int32 max_width = -1;
    int32 max_height = -1;

    for(uint32 image_index = 0;
        image_index < at->elements_len;
        ++image_index) {

        if (images.count >= images.capacity) {
            images.capacity = iamges.capacity * 2 + 1;
            images.items = realloc(images.items,
                                   images.capacity * sizeof(RY_DataImage));
            if (images.items == NULL) {
                ry->err = RY_ERR_MEMORY_ALLOCATION;
                return;
            }
        }
        
        RY_DataImage *new_image = &images.items[count++];
        new_image->path = at->elements[image_index].filename;
        // NOTE: Need to free this data later
        uint8 *image_data = stbi_load(new_image->path,
                                        &new_image->width, &new_image->height,
                                        &new_image->nr_channels, 0);
        printf("Loading image (%s) data from file\n",
                at->elements[image_index].filename);

        if (new_image->width > max_width) max_width = new_image->width;
        if (new_image->height > max_height) max_height = new_image->height;

        // Generate texture
        if (image_data) {
            new_image->data = image_data;
        } else {
            printf("[ERROR] Failed to load image: %s\n", new_image->path);
            ry->err = RY_ERR_FAILED_IO;
            return;
        }
    }

    // Get GPU limits
    uint32 max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    uint32 max_array_texture_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_array_texture_layers);

    if (max_width > max_texture_size || max_height > max_texture_size) {
        printf("[ERROR] Failed to create array texture: max texture size (%d) is bigger than GL_MAX_TEXTURE_SIZE (%d)\n",
                ((max_width > max_height) ? max_width : max_height),
                max_texture_size);
        ry->err = RY_ERR_TEXTURE_SIZE;
        return;
    }

    if ((uint32)images.count > max_array_texture_layers) {
        printf("[ERROR] Failed to create array texture: number of textures (%d) is bigger than GL_MAX_ARRAY_TEXTURE_LAYERS (%d)",
                images.count,
                max_array_texture_layers);
        ry->err = RY_ERR_TEXTURE_LAYER;
        return;
    }

    at->elements = (RY_TextureElement *)
        malloc(sizeof(RY_TextureElement) * images.count);
    if (at->elements == NULL) {
        ry->err = RY_ERR_MEMORY_ALLOCATION;
        return;
    }

    glGenTextures(1, &at->id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, at->id);

    glTexStorage3D(
        GL_TEXTURE_2D_ARRAY, // GLenum target
        1, // GLsizei levels
        GL_RGBA8, // GLenum internalformat
        max_width, // GLsizei width
        max_height, // GLsizei height
        images.count // GLsizei depth
    );

    for(uint32 image_index = 0;
        image_index < images.count;
        ++image_index) {

        RY_DataImage *image = &(images.items[image_index]);
        RY_TextureElement *t_element = &(at->elements[image_index]);
        t_element->width = image->width;
        t_element->height = image->height;
        t_element->tex_coords = {
            .tx = 0,
            .ty = 0,
            .tw = (float) t_element->width / max_width,
            .th = (float) t_element->height / max_height,
        };

        printf("Loading image (%s) data into the texture\n", image->path);

        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY, // GLenum target
            0, // GLint level
            0, // GLint xoffset
            0, // GLint yoffset
            image_index, // GLint zoffset
            image->width, // GLsizei width
            image->height, // GLsizei height
            1, // GLsizei depth
            GL_RGBA, // GLenum format
            GL_UNSIGNED_BYTE, // GLenum type
            image->data // const GLvoid * pixels
        );

        // Don't need it anymore, because the data is inside the texture now
        stbi_image_free(image->data);
        image->data = NULL;
    }

    // texture options
    // TODO(gio): personalize texture options?
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
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

RY_ShaderProgram ry_shader_create_program(
        RY_Rendy *ry,
        const char *vertex_shader_path,
        const char *fragment_shader_path) {
    RY_ShaderProgram program = 0;

    // Reading the vertex shader code
    FILE *vertex_file = fopen(vertex_shader_path, "rb");
    if (vertex_file == NULL) {
        ry->err = RY_ERR_IO_COULD_NOT_OPEN;
        return program;
    }

    if (fseek(vertex_file, 0, SEEK_END)) {
        ry->err = RY_ERR_IO_COULD_NOT_SEEK;
        return program;
    }
    uint64 vertex_file_size = ftell(vertex_file);
    if (fseek(vertex_file, 0, SEEK_SET)) {
        ry->err = RY_ERR_IO_COULD_NOT_SEEK;
        return program;
    }

    uint8 *vertex_code = malloc(vertex_file_size + 1);
    if (vertex_code == NULL) {
        ry->err = RY_ERR_MEMORY_ALLOCATION;
        return program;
    }

    if (fread(vertex_code, vertex_file_size, 1, vertex_file) != 1) {
        ry->err = RY_ERR_IO_COULD_NOT_READ;
        return program;
    }
    fclose(vertex_file);

    vertex_code[vertex_file_size] = '\0';

    // Reading the fragment shader code
    FILE *fragment_file = fopen(fragment_shader_path, "rb");
    if (fragment_file == NULL) {
        ry->err = RY_ERR_IO_COULD_NOT_OPEN;
        return program;
    }

    if (fseek(fragment_file, 0, SEEK_END)) {
        ry->err = RY_ERR_IO_COULD_NOT_SEEK;
        return program;
    }
    uint64 fragment_file_size = ftell(fragment_file);
    if (fseek(fragment_file, 0, SEEK_SET)) {
        ry->err = RY_ERR_IO_COULD_NOT_SEEK;
        return program;
    }

    uint8 *fragment_code = malloc(fragment_file_size + 1);
    if (fragment_code == NULL) {
        ry->err = RY_ERR_MEMORY_ALLOCATION;
        return program;
    }

    if (fread(fragment_code, fragment_file_size, 1, fragment_file) != 1) {
        ry->err = RY_ERR_IO_COULD_NOT_READ;
        return program;
    }
    fclose(fragment_file);

    fragment_code[fragment_file_size] = '\0';

    // compile shaders and create program
    uint32 vertex, fragment;
    int32 success;
    char err_log[512];

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_code, NULL);
    glCompileShader(vertex);
    // print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex, 512, NULL, err_log);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED (%s)\n%s\n",
                vertex_shader_path, err_log);
        ry->err = RY_ERR_SHADER_COULD_NOT_COMPILE;
        return program;
    }

    // fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_code, NULL);
    glCompileShader(fragment);
    //print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment, 512, NULL, err_log);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED (%s)\n%s\n",
                fragment_shader_path, err_log);
        ry->err = RY_ERR_SHADER_COULD_NOT_COMPILE;
        return program;
    }

    // program
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    // print linking errors if any
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, 512, NULL, err_log);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n", err_log);
        ry->err = RY_ERR_SHADER_COULD_NOT_LINK;
        return program;
    }

    // delete the single shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    ry->err = RY_ERR_NONE;
    return program;
}

void ry_shader_set_int32(
        RY_ShaderProgram s,
        const char* name,
        int32 value) {
}

void ry_shader_set_float(
        RY_ShaderProgram s,
        const char* name,
        float value) {
}

void ry_shader_set_mat4f(
        RY_ShaderProgram s,
        const char* name,
        mat4f value) {
}

void ry_shader_set_vec3f(
        RY_ShaderProgram s,
        const char* name,
        vec3f value) {
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
    } else if (ry->err == RY_ERR_OUT_OF_BUFFER_MEMORY) {
        return "Ran out of buffer memory";
    } else if (ry->err == RY_ERR_TEXTURE_SIZE) {
        return "TextureArray component size is greater than hardware limit";
    } else if (ry->err == RY_ERR_TEXTURE_LAYER) {
        return "TextureArray layers number is greater than hardware limit";
    } else if (ry->err == RY_ERR_FAILED_IO) {
        return "Failed to read file";
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
