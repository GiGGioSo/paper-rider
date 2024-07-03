#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "../include/glad/glad.h"
#include "../include/glfw3.h"
#include "../include/glm/ext.hpp"

#include "pp_globals.h"
#include "pp_renderer.h"
#include "pp_game.h"

// Callbacks
void callback_framebuffer_size(GLFWwindow *window, int width, int height);
void callback_debug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void* user);
void callback_gamepad(int gamepad_id, int event);

// Initializing global structure
PR *glob = NULL;
int glob_init();
void glob_free();

float last_frame = 0.f;
float this_frame = 0.f;

int fps_to_display;
int fps_counter;
float time_from_last_fps_update;

int main() {
    std::srand(std::time(nullptr));

    glob = (PR *) std::malloc(sizeof(PR));
    glob->window.title = "PaperPlane";
    glob->window.display_mode = PR::WINDOWED;
    // These values are used only if display_mode == PR::WINDOWED
    glob->window.width = 1200;
    glob->window.height = 900;
    glob->window.windowed_resolution = PR::R1200x900;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (glob->window.display_mode == PR::FULLSCREEN) {
        GLFWmonitor* main_monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(main_monitor);

        glob->window.glfw_win =
            glfwCreateWindow(mode->width, mode->height, glob->window.title,
                             glfwGetPrimaryMonitor(), NULL);
    } else if (glob->window.display_mode == PR::BORDERLESS) {
        glfwWindowHint(GLFW_DECORATED, 0);

        GLFWmonitor* main_monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(main_monitor);
        glob->window.width = mode->width;
        glob->window.height = mode->height;

        glob->window.glfw_win =
            glfwCreateWindow(glob->window.width, glob->window.height,
                             glob->window.title, NULL, NULL);
    } else if (glob->window.display_mode == PR::WINDOWED) {
        glob->window.glfw_win =
            glfwCreateWindow(glob->window.width, glob->window.height,
                             glob->window.title, NULL, NULL);
    } else {
        std::cout << "[ERROR] Unknown window mode: "
                  << glob->window.display_mode << std::endl;
    }
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
    glfwSetJoystickCallback(callback_gamepad);
    glDebugMessageCallback(&callback_debug, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    // Get the actual size of the window that got created
    glfwGetFramebufferSize(glob->window.glfw_win,
                           &glob->window.width, &glob->window.height);
    // Manually call the callback to set correct glViewPort and blackbars
    callback_framebuffer_size(glob->window.glfw_win,
                              glob->window.width,
                              glob->window.height);

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
                      << (int) glob->input.current_gamepad
                      << ", Name: "
                      << (glob->input.gamepad_name ?
                               glob->input.gamepad_name : "none")
                      << std::endl;

        }

        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // NOTE: Update input
        InputController *input = &glob->input;
        input_controller_update(glob->window.glfw_win, input,
                                glob->window.vertical_bar,
                                glob->window.horizontal_bar,
                                glob->window.width,
                                glob->window.height);
        if (ACTION_CLICKED(PR_EXIT_GAME)) {
            glfwSetWindowShouldClose(glob->window.glfw_win, true);
        }


        switch (glob->state.current_case) {
            case PR::START_MENU:
            {
                start_menu_update();
                break;
            }
            case PR::PLAY_MENU:
            {
                play_menu_update();
                break;
            }
            case PR::OPTIONS_MENU:
            {
                options_menu_update();
                break;
            }
            case PR::LEVEL:
            {
                level_update();
                break;
            }
            default:
            {
                std::cout << "Unknown state: "
                          << glob->state.current_case
                          << std::endl;
            }
        }

        glfwSwapBuffers(glob->window.glfw_win);
        glfwPollEvents();
    }

    glob_free();

    glfwTerminate();
    return 0;
}

int glob_init(void) {
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

    // ### Rendering initialization ###
    glob->rend_res.ortho_proj = glm::ortho(0.0f, (float)GAME_WIDTH,
                                       (float)GAME_HEIGHT, 0.0f);

    InputController *in = &glob->input;
    input_controller_init(in);

    // NOTE: Initializing of the shaders
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

    Shader *s4 = &glob->rend_res.shaders[3];
    shaderer_create_program(s4, "./res/shaders/text_wave.vs",
                            "./res/shaders/text_default.fs");
    shaderer_set_mat4(*s4, "projection",
                      glob->rend_res.ortho_proj);

    Shader *s5 = &glob->rend_res.shaders[4];
    shaderer_create_program(s5, "./res/shaders/tex_array.vs",
                            "./res/shaders/tex_array.fs");
    shaderer_set_mat4(*s5, "projection",
                      glob->rend_res.ortho_proj);

    // NOTE: Initializing the global_sprite
    renderer_create_texture(&glob->rend_res.global_sprite,
                            "res/paper-rider_sprite3.png");

    std::cout << "Loaded the spritesheet of size: "
              << glob->rend_res.global_sprite.width
              << "x"
              << glob->rend_res.global_sprite.height
              << std::endl;


    // # Font initialization
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
        return 1;
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
        return 1;
    }

    Font *f3 = &glob->rend_res.fonts[ACTION_NAME_FONT];
    f3->filename = "./arial.ttf";
    f3->first_char = 32;
    f3->num_chars = 96;
    f3->font_height = ACTION_NAME_FONT_SIZE;
    f3->bitmap_width = 512;
    f3->bitmap_height = 512;
    f3->char_data = (stbtt_bakedchar*) std::malloc(sizeof(stbtt_bakedchar) *
                                                   f3->num_chars);
    error = renderer_create_font_atlas(f3);
    if (error) {
        std::cout << "[ERROR] Could not create font atlas: "
                  << error << std::endl;
        return 1;
    }
    
    // # Array textures initialization
    ArrayTexture *at1 = &glob->rend_res.array_textures[0];
    at1->elements_len = PR_LAST_TEX1 + 1;
    at1->elements = (TextureElement *) std::malloc(sizeof(TextureElement) * at1->elements_len);
    // Elements initialization
    at1->elements[PR_TEX1_FRECCIA] = { .filename = "res/test_images/freccia.png" };
    renderer_create_array_texture(at1);

    ArrayTexture *at2 = &glob->rend_res.array_textures[1];
    at2->elements_len = PR_LAST_TEX2 + 1;
    at2->elements = (TextureElement *) std::malloc(sizeof(TextureElement) * at2->elements_len);
    // Elements initialization
    at2->elements[PR_TEX2_PLANE] = { .filename = "res/test_images/plane.png" };
    renderer_create_array_texture(at2);

    // # GPU resources allocation
    renderer_init(&glob->renderer);

    // ### Obstacle colors initialization
    glob->colors[0] = glm::vec4(0.8f, 0.3f, 0.3f, 1.0f);
    glob->colors[1] = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glob->colors[2] = glm::vec4(0.3f, 0.3f, 0.8f, 1.0f);
    glob->colors[3] = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

    // ### Sound initialization ###
    PR::Sound *sound = &glob->sound;
    sound->master_volume = 1.f;
    sound->sfx_volume = 1.f;
    sound->music_volume = 1.f;
    // Engine
    ma_result result;
    result = ma_engine_init(NULL, &sound->engine);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the audio engine!"
                  << std::endl;
        return 1;
    }

    // Sound groups
    result = ma_sound_group_init(&sound->engine, 0, NULL,
                                 &sound->music_group);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound music group!"
                  << std::endl;
        return 1;
    }
    result = ma_sound_group_init(&sound->engine, 0, NULL,
                                 &sound->sfx_group);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound sfx group!"
                  << std::endl;
        return 1;
    }

    // Sounds in the music group
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/menu_theme.wav",
                                     MA_SOUND_FLAG_STREAM,
                                     &sound->music_group, NULL,
                                     &sound->menu_music);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound menu_music!"
                  << std::endl;
        return 1;
    }
    ma_sound_set_looping(&sound->menu_music, true);

    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/menu_theme.wav",
                                     MA_SOUND_FLAG_STREAM,
                                     &sound->music_group, NULL,
                                     &sound->playing_music);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound playing_music!"
                  << std::endl;
        return 1;
    }

    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/menu_theme.wav",
                                     MA_SOUND_FLAG_STREAM,
                                     &sound->music_group, NULL,
                                     &sound->gameover_music);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound gameover_music!"
                  << std::endl;
        return 1;
    }

    // Sounds in the sfx group
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/menu_select.wav",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->change_selection);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound change_selection!"
                  << std::endl;
        return 1;
    }
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/menu_select.wav",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->click_selected);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound click_selected!"
                  << std::endl;
        return 1;
    }
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/menu_select.wav",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->change_pane);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound change_pane!"
                  << std::endl;
        return 1;
    }
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/menu_select.wav",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->to_start_menu);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound campaign_custom!"
                  << std::endl;
        return 1;
    }
    /*
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/discord-join.mp3",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->rider_detach);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound rider_detach!"
                  << std::endl;
        return 1;
    }
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/discord-notification.mp3",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->rider_double_jump);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound rider_double_jump!"
                  << std::endl;
        return 1;
    }
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/succ.mp3",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->plane_crash);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound plane_crash!"
                  << std::endl;
        return 1;
    }
    result = ma_sound_init_from_file(&sound->engine,
                                     "./res/sounds/rider_crash.wav",
                                     MA_SOUND_FLAG_DECODE,
                                     &sound->sfx_group, NULL,
                                     &sound->rider_crash);
    if (result != MA_SUCCESS) {
        std::cout << "[ERROR] Could not initialize the sound rider_crash!"
                  << std::endl;
        return 1;
    }
    */

    std::cout << "Loaded all audio files successfully!" << std::endl;

    start_menu_set_to_null(&glob->current_start_menu);
    play_menu_set_to_null(&glob->current_play_menu);
    options_menu_set_to_null(&glob->current_options_menu);
    level_set_to_null(&glob->current_level);

    glob->current_start_menu.selection = PR::START_BUTTON_PLAY;
    CHANGE_CASE_TO_START_MENU(0);

    return 0;
}

void glob_free(void) {
    PR::Sound *s = &glob->sound;
    ma_sound_uninit(&s->menu_music);
    ma_sound_uninit(&s->playing_music);
    ma_sound_uninit(&s->gameover_music);
    ma_sound_uninit(&s->change_selection);
    ma_sound_uninit(&s->click_selected);
    ma_sound_uninit(&s->change_pane);
    ma_sound_uninit(&s->to_start_menu);
    ma_sound_uninit(&s->rider_detach);
    ma_sound_uninit(&s->rider_double_jump);
    ma_sound_uninit(&s->plane_crash);
    ma_sound_uninit(&s->rider_crash);
    ma_sound_group_uninit(&s->music_group);
    ma_sound_group_uninit(&s->sfx_group);
    ma_engine_uninit(&s->engine);
    for(size_t font_index = 0;
        font_index < ARRAY_LENGTH(glob->rend_res.fonts);
        ++font_index) {
        std::free(glob->rend_res.fonts[font_index].char_data);
    }
    for(size_t array_texture_index = 0;
        array_texture_index < ARRAY_LENGTH(glob->rend_res.array_textures);
        ++array_texture_index) {
        std::free(glob->rend_res.array_textures[array_texture_index].elements);
    }
    std::free(glob);
}

void callback_gamepad(int gamepad_id, int event) {
    InputController *in = &glob->input;

    if (event == GLFW_CONNECTED) {
        if (in->current_gamepad == -1 && glfwJoystickIsGamepad(gamepad_id)) {
            in->current_gamepad = gamepad_id;
            in->gamepad_name = (char *)glfwGetGamepadName(gamepad_id);
        } // else we don't care, how current controller is still good to go
    }
    else if (event == GLFW_DISCONNECTED) {
        if (gamepad_id == in->current_gamepad) {
            in->current_gamepad = -1;
            in->gamepad_name = NULL;
            // NOTE: Try to find another controller
            for(size_t jid = 0; jid < GLFW_JOYSTICK_LAST; ++jid) {
                if (glfwJoystickIsGamepad(jid)) {
                    in->current_gamepad = jid;
                    in->gamepad_name = (char *)glfwGetGamepadName(jid);
                    break;
                }
            }
        } // else we don't care, how current controller is still good to go
    }
}

void callback_framebuffer_size(GLFWwindow* window,
                               int width, int height) {
    UNUSED(window);

    PR::WinInfo *win = &glob->window;

    int height_from_width = width * 3 / 4;
    int width_from_height = height * 4 / 3;

    if (height_from_width == height) {
        win->vertical_bar = 0;
        win->horizontal_bar = 0;
        win->width = width;
        win->height = height;
    } else if (height_from_width > height) { // vertical bars
        win->vertical_bar = (width+1 - width_from_height) / 2;
        win->horizontal_bar = 0;
        win->width = width_from_height;
        win->height = height;
    } else if (height_from_width < height) { // horizontal bars
        win->vertical_bar = 0;
        win->horizontal_bar = (height+1 - height_from_width) / 2;
        win->width = width;
        win->height = height_from_width;
    }
    glViewport(win->vertical_bar, win->horizontal_bar,
               win->width, win->height);
}

void callback_debug(GLenum source,
                    GLenum type,
                    GLuint id, GLenum severity,
                    GLsizei length, const GLchar* message,
                    const void* user) {
    UNUSED(id);
    UNUSED(severity);
    UNUSED(length);
    UNUSED(user);

    std::cout << "------------------------------"
              << "\nSource: " << source
              << "\nType: " << type
              << "\nMessage: " << message
              << "\n-----------------------------";
}
