#include "../include/glad/glad.h"
#include "../include/glfw3.h"
#include "../include/glm/ext.hpp"

#include "pp_globals.h"
#include "shaderer.h"
#include "quad_renderer.h"

#include <iostream>

// Callbacks
void callback_framebuffer_size(GLFWwindow* window,
                               int width, int height);
void callback_debug(GLenum source,
                    GLenum type,
                    GLuint id, GLenum severity,
                    GLsizei length, const GLchar* message,
                    const void* user);

// Initializing global structure
PP* glob = (PP*) malloc(sizeof(PP));
void glob_init();
void glob_free();

// TODO: Probably create a "game" source and header file for these functions
void game_draw();

int main() {

    glob->window.title = "PaperPlane";
    glob->window.w = 800;
    glob->window.h = 600;

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
        glClearColor(0.3f, 0.8f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        game_draw();

        glfwSwapBuffers(glob->window.glfw_win);
        glfwPollEvents();
    }

    glob_free();

    glfwTerminate();
    return 0;
}

void glob_init(void) {
    PP::WinInfo* win = &glob->window;

    glob->ortho_proj = glm::ortho(0.0f, (float)win->w,
                                  (float)win->h, 0.0f,
                                  -1.f, 1.f);

    // NOTE: 1 is the number of shaders
    glob->shaders = (Shader*) malloc(sizeof(Shader) * 1);
    Shader* s1 = &glob->shaders[0];
    shaderer_create_program(s1, "res/shaders/quad_default.vs", "res/shaders/quad_default.fs");
    shaderer_set_mat4(*s1, "projection", glob->ortho_proj);

    quad_render_init(&glob->quad_renderer);

    PP::Plane* p = &glob->plane;
    p->x = win->w / 2.f;
    p->y = win->h / 2.f;
    p->w = 180.f;
    p->h = 10.f;
    p->rotation = 10.f;
}

void game_draw(void) {
    PP::Plane* p = &glob->plane;
    quad_render_add_queue(p->x, p->y, p->w, p->h, p->rotation, glm::vec3(1.0f, 1.0f, 1.0f), true);

    quad_render_draw(glob->shaders[0]);
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
