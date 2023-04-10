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
void callback_joystick(int joystick_id, int event);

// Initializing global structure
PR* glob = NULL;
void glob_init();
void glob_free();

float last_frame = 0.f;
float this_frame = 0.f;

int fps_to_display;
int fps_counter;
float time_from_last_fps_update;

int main() {
    srand(time(NULL));

    glob = (PR *) std::malloc(sizeof(PR));
    glob->window.title = "PaperPlane";
    glob->window.w = (uint32_t) (SCREEN_WIDTH_PROPORTION * 3.5f);
    glob->window.h = (uint32_t) (SCREEN_HEIGHT_PROPORTION * 3.5f);

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

    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_GEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Callbacks
    glfwSetFramebufferSizeCallback(glob->window.glfw_win,
                                   callback_framebuffer_size);
    glfwSetJoystickCallback(callback_joystick);
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
            std::cout << "Controller: "
                      << (int) glob->input.current_joystick
                      << std::endl;

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
            case PR::LEVEL:
            {
                level_update();
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
    // NOTE: Set custom cursor
    uint8_t cursor_pixels[16 * 16 * 4];
    for(size_t row = 0; row < 16; ++row) {
        for(size_t col = 0; col < 16; ++col) {
            size_t pixel = (row*16+col) * 4;
            if (row + col < 16) {
                // NOTE: Alpha-Blue-Green-Red
                *((uint32_t *) &cursor_pixels[pixel]) = 0xff333333;
            } else {
                // NOTE: Alpha-Blue-Green-Red
                *((uint32_t *) &cursor_pixels[pixel]) = 0x00;
            }
        }
    }

    GLFWimage image;
    image.width = 16;
    image.height = 16;
    image.pixels = cursor_pixels;
    GLFWcursor *cursor = glfwCreateCursor(&image, 0, 0);
    // Don't really need to check if the cursor is NULL,
    // because if it is, then the cursor will be set to default
    glfwSetCursor(glob->window.glfw_win, cursor);


    glob->state.current_case = PR::MENU;

    // NOTE: I need to set it here because I do not reset
    //       it when preparing the menu, so that the player comes back to
    //       where he was before
    glob->current_menu.showing_campaign_buttons = true;
    menu_prepare(&glob->current_menu, &glob->current_level, "");

    PR::WinInfo* win = &glob->window;

    // Rendering
    glob->rend_res.ortho_proj = glm::ortho(0.0f, (float)win->w,
                                       (float)win->h, 0.0f);

    glob->input.current_joystick = -1;

    // NOTE: Initializing of the shaders
    /* glob->rend.shaders = (Shader *) malloc(sizeof(Shader) * 2); */
    Shader *s1 = &glob->rend_res.shaders[0];
    shaderer_create_program(s1, "./res/shaders/quad_default.vs",
                            "./res/shaders/quad_default.fs");
    shaderer_set_mat4(*s1, "projection",
                      glob->rend_res.ortho_proj);

    Shader *s2 = &glob->rend_res.shaders[1];
    shaderer_create_program(s2, "./res/shaders/tex_default.vs",
                            "./res/shaders/tex_default.fs");
    shaderer_set_mat4(*s2, "projection",
                      glob->rend_res.ortho_proj);

    Shader *s3 = &glob->rend_res.shaders[2];
    shaderer_create_program(s3, "./res/shaders/text_default.vs",
                            "./res/shaders/text_default.fs");
    shaderer_set_mat4(*s3, "projection",
                      glob->rend_res.ortho_proj);

    // NOTE: Initializing the global_sprite
    renderer_create_texture(&glob->rend_res.global_sprite,
                            "res/paper-rider_sprite.png");

    int error = 0;
    Font *f1 = &glob->rend_res.fonts[DEFAULT_FONT];
    f1->filename = "./arial.ttf";
    f1->first_char = 32;
    f1->num_chars = 96;
    f1->font_height = DEFAULT_FONT_SIZE;
    f1->bitmap_width = 512;
    f1->bitmap_height = 512;
    f1->char_data = (stbtt_bakedchar*) std::malloc(sizeof(stbtt_bakedchar) *
                                                   f1->num_chars);
    error = renderer_create_font_atlas(f1);
    if (error) {
        std::cout << "[ERROR] Could not create font atlas: "
                  << error << std::endl;
    }


    Font *f2 = &glob->rend_res.fonts[OBJECT_INFO_FONT];
    f2->filename = "./arial.ttf";
    f2->first_char = 32;
    f2->num_chars = 96;
    f2->font_height = OBJECT_INFO_FONT_SIZE;
    f2->bitmap_width = 512;
    f2->bitmap_height = 512;
    f2->char_data = (stbtt_bakedchar*) std::malloc(sizeof(stbtt_bakedchar) *
                                                   f2->num_chars);
    error = renderer_create_font_atlas(f2);
    if (error) {
        std::cout << "[ERROR] Could not create font atlas: "
                  << error << std::endl;
    }

    renderer_init(&glob->renderer);

    glob->colors[0] = glm::vec4(0.8f, 0.3f, 0.3f, 1.0f);
    glob->colors[1] = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glob->colors[2] = glm::vec4(0.3f, 0.3f, 0.8f, 1.0f);
    glob->colors[3] = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
}

void glob_free(void) {
    std::free(glob->rend_res.fonts[0].char_data);
    std::free(glob);
}

void callback_joystick(int joystick_id, int event) {
    InputController *in = &glob->input;

    if (event == GLFW_CONNECTED) {
        if (in->current_joystick == -1) {
            in->current_joystick = joystick_id;
        } // else we don't care, how current controller is still good to go
    }
    else if (event == GLFW_DISCONNECTED) {
        if (joystick_id == in->current_joystick) {
            in->current_joystick = -1;
            // NOTE: Try to find another controller
            for(size_t jid = 0;
                jid < GLFW_JOYSTICK_LAST;
                ++jid) {

                if (glfwJoystickPresent(jid)) {
                    in->current_joystick = jid;
                    break;
                }
            }
        } // else we don't care, how current controller is still good to go
    }
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
