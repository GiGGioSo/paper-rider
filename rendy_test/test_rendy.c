#include <stdio.h>

#include "../include/glad/glad.h"
#include "../include/glfw3.h"

#include "../src/pr_mathy.h"
#define RENDY_IMPLEMENTATION
#include "../src/pr_rendy.h"

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

    uint32 vertex_info[] = {
        GL_FLOAT, sizeof(float), 2, // position
        GL_FLOAT, sizeof(float), 4, // color
    };
    uint32 max_vertex_number = 6;

    // Create a target VAO
    RY_Target target = ry_create_target(ry, vertex_info, 6, max_vertex_number);
    if (ry_error(ry)) {
        fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
        return ry_error(ry);
    }

    // Create a shader program
    RY_ShaderProgram program = ry_shader_create_program(
            ry,
            "shaders/default.vs",
            "shaders/default.fs");
    if (ry_error(ry)) {
        fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
        return ry_error(ry);
    }

    // Create rendering layer
    uint32 layer_index = ry_register_layer(ry, 1, program, NULL, target, 0);
    if (ry_error(ry)) {
        fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
        return ry_error(ry);
    }

    float this_frame = 0.f;
    float last_frame = 0.f;
    float time_from_last_fps_update = 0.f;

    float vertices[] = {
        -0.5f, -0.5f, 1.f, 0.f, 0.f, 1.f,
        -0.5f, 0.5f, 0.f, 1.f, 0.f, 1.f,
        0.5f, 0.5f, 0.f, 0.f, 1.f, 1.f,
        0.5f, -0.5f, 0.f, 0.f, 0.f, 1.f
    };

    uint32 indices[] = {
        0, 1, 2,
        0, 2, 3
    };

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

        ry_push_polygon(
                ry,
                layer_index, 0,
                (void *) vertices, 4,
                indices, 6
                );
        if (ry_error(ry)) {
            fprintf(stderr, "[ERROR] Rendy: %s\n", ry_err_string(ry));
            break;
        }

        ry__draw_layer(ry, layer_index);

        glfwSwapBuffers(glfw_win);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
