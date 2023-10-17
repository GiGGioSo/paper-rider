#ifndef PP_INPUT_H
#define PP_INPUT_H

#include "../include/glfw3.h"
#include "../include/glm/vec2.hpp"

#define GAMEPAD_DEADZONE (0.3f)

struct Key {
    bool old;
    bool clicked;
    bool pressed;
};

enum GamepadBindingType {
    BUTTON = 0,
    AXIS_POSITIVE = 1,
    AXIS_NEGATIVE = 2,
};
struct GamepadBinding {
    GamepadBindingType type;
    int bind_index;
};

struct KeyboardBinding {
    int bind_index;
};

struct InputAction {
    Key key;

    // Analog value of the action, always between 0 and 1
    float value;

    KeyboardBinding kb_binds[2];
    GamepadBinding gp_binds[2];
};

inline
void key_reset(Key *key) {
    key->old = key->pressed;
    key->pressed = false;
    key->clicked = false;
}

inline
void key_pressed(Key *key) {
    key->pressed = true;
    if (key->old == false) {
        key->clicked = true;
    } else {
        key->clicked = false;
    }
}

struct InputController {

    // ### NEW STUFF ###
    InputAction actions[1];

    // ### OLD STUFF ###

    // Gameplay
    int8_t current_gamepad;
    char *gamepad_name;

    // NOTE: Global
    Key exit;

    // NOTE: Mouse
    Key mouse_left;
    Key mouse_right;
    Key mouse_middle;
    double mouseX;
    double mouseY;
    double old_mouseX;
    double old_mouseY;
    bool was_mouse_moved;

    // NOTE: Gameplay
    float left_right;
    float up_down;
    Key boost;
    Key jump;

    Key pause;
    Key resume;
    Key restart;
    Key quit;

    // NOTE: Editing
    Key edit;
    Key save_map;
    Key obj_delete;
    Key obj_add;
    Key obj_duplicate;
    Key reset_pos;

    // NOTE: Menu
    Key menu_up;
    Key menu_down;
    Key menu_left;
    Key menu_right;
    Key menu_click;
    Key menu_custom_delete;
    Key menu_custom_edit;
    Key menu_pane_right;
    Key menu_pane_left;
    Key menu_to_start_menu;

    // NOTE: Debug
    Key debug;
    Key up;
    Key down;
    Key left;
    Key right;
};

/* Updates the InputController global struct
 *
 */
void input_controller_update(GLFWwindow *window, InputController *input, const int vertical_bar, const int horizontal_bar, const int screen_w, const int screen_h);

#endif
