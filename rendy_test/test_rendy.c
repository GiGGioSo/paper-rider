#include <stdio.h>

#include "../include/glad/glad.h"
#include "../include/glfw3.h"

#include "../src/pr_mathy.h"

#define RENDY_IMPLEMENTATION
#include "../src/pr_rendy.h"

int create_uni_layer(RY_Rendy *ry, uint32 sort_index);
int create_textured_layer(RY_Rendy *ry, uint32 sort_index);
void push_figure(RY_Rendy *ry, uint32 layer_index, uint32 texture_layer, float x, float y, float w, float h);

GLFWwindow *glfw_win;
float delta_time;
uint64 fps_counter;

RY_Rendy *ry;

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfw_win = glfwCreateWindow(800, 600, "RENDY TEST", NULL, NULL);

    if (glfw_win == NULL) {
        fprintf(stderr, "[ERROR] Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(glfw_win);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fprintf(stderr, "[ERROR] Failed to initialize GLAD\n");
        return 1;
    }

    // Init rendy
    ry = ry_init();
    if (ry == NULL) {
        fprintf(stderr, "Could not initialize Rendy!\n");
        glfwTerminate();
        return 1;
    }
    ry_gl_enable(GL_BLEND);
    ry_gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // uni rendering initialization
    int uni_layer = create_uni_layer(ry, 3);
    if (ry_error(ry)) {
        fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
        glfwTerminate();
        return ry_error(ry);
    }
    float uni_quad[] = {
        -0.5f, -0.5f, 1.f, 0.f, 0.f, 1.f,
        -0.5f, 0.f, 0.f, 1.f, 0.f, 1.f,
        0.f, 0.f, 0.f, 0.f, 1.f, 1.f,
        0.f, -0.5f, 0.f, 0.f, 0.f, 1.f
    };
    float uni_triangle[] = {
        0.25f, 0.f, 1.f, 0.f, 0.f, 1.f,
        0.f, 0.5f, 0.f, 1.f, 0.f, 1.f,
        0.5f, 0.5f, 0.f, 0.f, 1.f, 1.f,
    };

    // textured rendering initialization
    int textured_layer = create_textured_layer(ry, 2);
    if (ry_error(ry)) {
        fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
        glfwTerminate();
        return ry_error(ry);
    }
    uint32 freccia_layer = 0;
    uint32 plane_layer = 1;

    uint32 quad_indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    float this_frame = 0.f;
    float last_frame = 0.f;
    float time_from_last_fps_update = 0.f;
    while (!glfwWindowShouldClose(glfw_win)) {
        this_frame = (float)glfwGetTime();
        delta_time = this_frame - last_frame;
        last_frame = this_frame;

        fps_counter++;
        time_from_last_fps_update += delta_time;
        if (time_from_last_fps_update > 1.f) {
            printf("FPS: %lu\n", fps_counter);
            fps_counter = 0;
            time_from_last_fps_update -= 1.f;
        }

        ry_gl_clear_color(make_vec4f(0.5f, 0.2f, 0.2f, 1.0f));
        ry_gl_clear(GL_COLOR_BUFFER_BIT);

        // push uni polygons
        ry_push_polygon(
                ry,
                uni_layer, 1,
                (void *) uni_quad, 4,
                quad_indices, 6);
        if (ry_error(ry)) {
            fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
            glfwTerminate();
            return ry_error(ry);
        }
        ry_push_polygon(
                ry,
                uni_layer, 2,
                (void *) uni_triangle, 3,
                NULL, 0);
        if (ry_error(ry)) {
            fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
            glfwTerminate();
            return ry_error(ry);
        }

        // push textured polygons
        push_figure(
                ry, textured_layer, freccia_layer,
                -0.25f, 0.f, 0.5f, 0.5f);
        if (ry_error(ry)) {
            fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
            glfwTerminate();
            return ry_error(ry);
        }
        push_figure(
                ry, textured_layer, plane_layer,
                -0.25f, 0.5f, 0.5f, 0.5f);
        if (ry_error(ry)) {
            fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
            glfwTerminate();
            return ry_error(ry);
        }

        // draw & reset both layers
        ry_draw_all_layers(ry);
        ry_reset_all_layers(ry);

        if (ry_error(ry)) {
            fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
            glfwTerminate();
            return ry_error(ry);
        }

        glfwSwapBuffers(glfw_win);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

int create_uni_layer(RY_Rendy *ry, uint32 sort_index) {
    uint32 vertex_info[] = {
        GL_FLOAT, sizeof(float), 2, // position
        GL_FLOAT, sizeof(float), 4, // color
    };
    uint32 max_vertex_number = 30;

    // Create a target VAO
    RY_Target target =
        ry_create_target(
                ry,
                vertex_info, 6,
                sizeof(uint32),
                max_vertex_number);
    if (ry_error(ry)) return ry_error(ry);

    // Create a shader program
    RY_ShaderProgram program =
        ry_shader_create_program(
                ry,
                "shaders/uni_default.vs",
                "shaders/uni_default.fs");
    if (ry_error(ry)) return ry_error(ry);

    // Create rendering layer
    uint32 layer_index = ry_register_layer(ry, sort_index, program, NULL, target, 0);
    if (ry_error(ry)) return ry_error(ry);

    return layer_index;
}

int create_textured_layer(RY_Rendy *ry, uint32 sort_index) {
    uint32 vertex_info[] = {
        GL_FLOAT, sizeof(float), 2,        // position
        GL_FLOAT, sizeof(float), 2,        // tex coordinates
        GL_UNSIGNED_INT, sizeof(uint32), 1 // layer index
    };
    uint32 max_vertex_number = 30;

    // Create a target VAO
    RY_Target target =
        ry_create_target(
                ry,
                vertex_info, 9,
                sizeof(uint32),
                max_vertex_number);
    if (ry_error(ry)) return ry_error(ry);

    // Create a shader program
    RY_ShaderProgram program =
        ry_shader_create_program(
                ry,
                "shaders/tex_array.vs",
                "shaders/tex_array.fs");
    if (ry_error(ry)) return ry_error(ry);

    // Create array texture
    const char *texture_paths[] = {
        "textures/freccia.png",
        "textures/plane.png"
    };
    RY_ArrayTexture array_texture = ry_create_array_texture(ry, texture_paths, 2);
    if (ry_error(ry)) return ry_error(ry);

    // Create rendering layer
    uint32 layer_index = ry_register_layer(
            ry, sort_index, program, &array_texture, target, RY_LAYER_TEXTURED);
    if (ry_error(ry)) return ry_error(ry);

    return layer_index;
}

void push_figure(
        RY_Rendy *ry,
        uint32 layer_index, uint32 texture_layer,
        float x, float y, float w, float h) {
    RY_TexCoords t = ry->layers.elements[layer_index].array_texture.elements[texture_layer].tex_coords;

    struct figure_vertex {
        float x, y, tx, ty;
        uint32 layer;
    };
    struct figure_vertex textured_figure[] = {
        { x, y,     t.tx, t.ty+t.th,      texture_layer },
        { x, y-h,   t.tx, t.ty,           texture_layer },
        { x+w, y-h, t.tx+t.tw, t.ty,      texture_layer },
        { x+w, y,   t.tx+t.tw, t.ty+t.th, texture_layer }
    };

    uint32 quad_indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    ry_push_polygon(
            ry,
            layer_index, 2,
            (void *) textured_figure, 4,
            quad_indices, 6);
}
