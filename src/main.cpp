#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "../include/glad/glad.h"
#include "../include/glfw3.h"
#include "../include/glm/ext.hpp"

#include "pp_globals.h"
#include "pp_shaderer.h"
#include "pp_renderer.h"
#include "pp_game.h"

// Callbacks
void callback_framebuffer_size(GLFWwindow* window,
                               int width, int height);
void callback_debug(GLenum source,
                    GLenum type,
                    GLuint id, GLenum severity,
                    GLsizei length, const GLchar* message,
                    const void* user);

// Initializing global structure
PR* glob;
void glob_init();
void glob_free();

float last_frame = 0.f;
float this_frame = 0.f;

int fps_to_display;
int fps_counter;
float time_from_last_fps_update;

int main() {
    srand(time(NULL));

    glob = (PR*) std::malloc(sizeof(PR));
    glob->window.title = "PaperPlane";
    glob->window.w = 1280;
    glob->window.h = 720;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glob->window.glfw_win =
        glfwCreateWindow(glob->window.w, glob->window.h,
                         glob->window.title, NULL, NULL);
    if (glob->window.glfw_win == NULL) {
        std::cout << "[ERROR] Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(glob->window.glfw_win);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "[ERROR] Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetInputMode(glob->window.glfw_win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Callbacks
    glfwSetFramebufferSizeCallback(glob->window.glfw_win,
                                   callback_framebuffer_size);
    glDebugMessageCallback(&callback_debug, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glViewport(0, 0, glob->window.w, glob->window.h);

    glob_init();

    while (!glfwWindowShouldClose(glob->window.glfw_win)) {

        this_frame = (float)glfwGetTime();
        glob->state.delta_time = this_frame - last_frame;
        last_frame = this_frame;

        fps_counter++;
        time_from_last_fps_update += glob->state.delta_time;
        if (time_from_last_fps_update > 1.f) {
            fps_to_display = fps_counter;
            fps_counter = 0;
            time_from_last_fps_update -= 1.f;

            // TODO: Debug flag
            std::cout << "FPS: " << fps_to_display << std::endl;
        }

        glClearColor(0.3f, 0.8f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // NOTE: Update input
        input_controller_update(glob->window.glfw_win, &glob->input);
        if (glob->input.exit.clicked) {
            glfwSetWindowShouldClose(glob->window.glfw_win, true);
        }

        switch (glob->state.current_case) {
            case PR::MENU:
            {
                menu_update();
                menu_draw();
                break;
            }
            case PR::LEVEL1:
            {
                level1_update();
                level1_draw();
                break;
            }
            case PR::LEVEL2:
            {
                level2_update();
                level2_draw();
                break;
            }
        }

        glfwSwapBuffers(glob->window.glfw_win);
        glfwPollEvents();
    }

    glob_free();

    glfwTerminate();
    return 0;
}

void glob_init(void) {
    glob->state.current_case = PR::MENU;
    menu_prepare(&glob->current_level);

    PR::WinInfo* win = &glob->window;

    // Rendering
    glob->rend_res.ortho_proj = glm::ortho(0.0f, (float)win->w,
                                       (float)win->h, 0.0f);

    // NOTE: Initializing of the shaders
    /* glob->rend.shaders = (Shader *) malloc(sizeof(Shader) * 2); */
    Shader *s1 = &glob->rend_res.shaders[0];
    shaderer_create_program(s1, "res/shaders/quad_default.vs",
                            "res/shaders/quad_default.fs");
    shaderer_set_mat4(*s1, "projection",
                      glob->rend_res.ortho_proj);

    Shader *s2 = &glob->rend_res.shaders[1];
    shaderer_create_program(s2, "res/shaders/tex_default.vs",
                            "res/shaders/tex_default.fs");
    shaderer_set_mat4(*s2, "projection",
                      glob->rend_res.ortho_proj);

    Shader *s3 = &glob->rend_res.shaders[2];
    shaderer_create_program(s3, "res/shaders/text_default.vs",
                            "res/shaders/text_default.fs");
    shaderer_set_mat4(*s3, "projection",
                      glob->rend_res.ortho_proj);

    // NOTE: Initializing the global_sprite
    renderer_create_texture(&glob->rend_res.global_sprite,
                            "res/paper-rider_sprite.png");

    Font *f1 = &glob->rend_res.fonts[0];
    f1->filename = "./arial.ttf";
    f1->first_char = 32;
    f1->num_chars = 96;
    f1->font_height = 64.0f;
    f1->bitmap_width = 1024;
    f1->bitmap_height = 1024;
    f1->char_data = (stbtt_bakedchar*) std::malloc(sizeof(stbtt_bakedchar) *
                                                   f1->num_chars);
    int error = renderer_create_font_atlas(f1);
    std::cout << error << std::endl;

    renderer_init(&glob->renderer);
}

void glob_free(void) {
    std::free(glob->rend_res.fonts[0].char_data);
    std::free(glob);
}

void callback_framebuffer_size(GLFWwindow* window,
                               int width, int height) {
    glViewport(0, 0, width, height);
    glob->rend_res.ortho_proj = glm::ortho(0.0f, (float)width,
                                       (float)height, 0.0f);
    for(size_t shader_index = 0;
        shader_index < ARRAY_LENGTH(glob->rend_res.shaders);
        ++shader_index) {

        shaderer_set_mat4(glob->rend_res.shaders[shader_index],
                          "projection", glob->rend_res.ortho_proj);
    }
    glob->window.w = width;
    glob->window.h = height;
}
void callback_debug(GLenum source,
                    GLenum type,
                    GLuint id, GLenum severity,
                    GLsizei length, const GLchar* message,
                    const void* user) {

    std::cout << "------------------------------"
              << "\nSource: " << source
              << "\nType: " << type
              << "\nMessage: " << message
              << "\n-----------------------------";
}
