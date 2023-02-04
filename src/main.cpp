#include "../include/glad/glad.h"
#include "../include/glfw3.h"
#include "../include/glm/ext.hpp"

#include "pp_globals.h"
#include "pp_shaderer.h"
#include "pp_quad_renderer.h"
#include "pp_game.h"
#include "pp_texturer.h"

#include <iostream>
#include <math.h>
#include <cstdio>

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


int main() {

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

    glob->window.glfw_win = glfwCreateWindow(glob->window.w, glob->window.h, glob->window.title, NULL, NULL);
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Callbacks
    glfwSetFramebufferSizeCallback(glob->window.glfw_win, callback_framebuffer_size);
    glDebugMessageCallback(&callback_debug, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glViewport(0, 0, glob->window.w, glob->window.h);


    glob_init();

    while (!glfwWindowShouldClose(glob->window.glfw_win)) {

        this_frame = (float)glfwGetTime();
        delta_time = this_frame - last_frame;
        last_frame = this_frame;


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
                level1_draw();
                break;
            }
            case PR::LEVEL2:
            {
                level2_update(delta_time);
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

    glob->current_state = PR::MENU;

    PR::WinInfo* win = &glob->window;

    // Rendering
    glob->rend.ortho_proj = glm::ortho(0.0f, (float)win->w,
                                  (float)win->h, 0.0f,
                                  -1.f, 1.f);

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

     // glob->current_level = (PR::Level *) malloc(sizeof(PR::Level));

}

void glob_free(void) {
    free(glob);
}

void callback_framebuffer_size(GLFWwindow* window,
                               int width, int height) {
    glViewport(0, 0, width, height);
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
