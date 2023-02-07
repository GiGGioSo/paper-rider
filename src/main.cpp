#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "../include/glad/glad.h"
#include "../include/glfw3.h"
#include "../include/glm/ext.hpp"

#include "pp_globals.h"
#include "pp_shaderer.h"
#include "pp_quad_renderer.h"
#include "pp_game.h"
#include "pp_texturer.h"

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

float last_frame = 0;
float this_frame = 0;
float delta_time = 0;

int fps_to_display;
int fps_counter;
float time_from_last_fps_update;

int main() {
    srand(time(NULL));

    glob = (PR*) malloc(sizeof(PR));
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
        delta_time = this_frame - last_frame;
        last_frame = this_frame;

        fps_counter++;
        time_from_last_fps_update += delta_time;
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
        if (glob->input.exit)
            glfwSetWindowShouldClose(glob->window.glfw_win, true);

        switch (glob->current_state) {
            case PR::MENU:
            {
                menu_update(delta_time);
                menu_draw();
                break;
            }
            case PR::LEVEL1:
            {
                level1_update(delta_time);
                level1_draw(delta_time);
                break;
            }
            case PR::LEVEL2:
            {
                level2_update(delta_time);
                level2_draw(delta_time);
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

    glob->current_state = PR::MENU;
    menu_prepare(&glob->current_level);

    PR::WinInfo* win = &glob->window;

    // Rendering
    glob->rend.ortho_proj = glm::ortho(0.0f, (float)win->w,
                                       (float)win->h, 0.0f);

    // NOTE: 1 is the number of shaders
    /* glob->rend.shaders = (Shader *) malloc(sizeof(Shader) * 2); */
    Shader *s1 = &glob->rend.shaders[0];
    shaderer_create_program(s1, "res/shaders/quad_default.vs", "res/shaders/quad_default.fs");
    shaderer_set_mat4(*s1, "projection", glob->rend.ortho_proj);

    Shader *s2 = &glob->rend.shaders[1];
    shaderer_create_program(s2, "res/shaders/tex_default.vs", "res/shaders/tex_default.fs");
    shaderer_set_mat4(*s2, "projection", glob->rend.ortho_proj);

    // NOTE: Initializing the global_sprite
    texturer_create_texture(&glob->rend.global_sprite, "res/paper-rider_sprite.png");

    quad_render_init(&glob->rend.quad_renderer);

}

void glob_free(void) {
    free(glob);
}

void callback_framebuffer_size(GLFWwindow* window,
                               int width, int height) {
    glViewport(0, 0, width, height);
    glob->rend.ortho_proj = glm::ortho(0.0f, (float)width,
                                       (float)height, 0.0f);
    for(size_t shader_index = 0;
        shader_index < ARRAY_LENGTH(glob->rend.shaders);
        ++shader_index) {

        shaderer_set_mat4(glob->rend.shaders[shader_index], "projection", glob->rend.ortho_proj);
    }
    glob->window.w = width;
    glob->window.h = height;
}
void callback_debug(GLenum source,
                    GLenum type,
                    GLuint id, GLenum severity,
                    GLsizei length, const GLchar* message,
                    const void* user) {

    std::cout << message << std::endl;

}
