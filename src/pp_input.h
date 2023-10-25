#ifndef PP_INPUT_H
#define PP_INPUT_H

#include "../include/glfw3.h"
#include "../include/glm/vec2.hpp"

// TODO: Put this as an option_slider to be modified in the keybindings pane
#define PR_GAMEPAD_DEADZONE (0.3f)

// Global actions
#define PR_EXIT_GAME 0

// In-Menu actions
#define PR_MENU_UP 1
#define PR_MENU_DOWN 2
#define PR_MENU_LEFT 3
#define PR_MENU_RIGHT 4
#define PR_MENU_CLICK 5
#define PR_MENU_LEVEL_DELETE 6
#define PR_MENU_LEVEL_EDIT 7
#define PR_MENU_PANE_RIGHT 8
#define PR_MENU_PANE_LEFT 9
#define PR_MENU_EXIT 10

// Gameplay actions
#define PR_PLAY_PLANE_UP 11
#define PR_PLAY_PLANE_DOWN 12
#define PR_PLAY_RIDER_RIGHT 13
#define PR_PLAY_RIDER_LEFT 14
#define PR_PLAY_RIDER_JUMP 15

// Editing actions
#define PR_EDIT_TOGGLE 16
#define PR_EDIT_SAVE_MAP 17
#define PR_EDIT_OBJ_DELETE 18
#define PR_EDIT_OBJ_CREATE 19
#define PR_EDIT_OBJ_DUPLICATE 20
#define PR_EDIT_PLANE_RESET 21
#define PR_EDIT_MOVE_UP 22
#define PR_EDIT_MOVE_DOWN 23
#define PR_EDIT_MOVE_LEFT 24
#define PR_EDIT_MOVE_RIGHT 25

#define PR_LAST_ACTION PR_EDIT_MOVE_RIGHT

struct Key {
    bool old;
    bool clicked;
    bool pressed;
};

enum GamepadBindingType {
    PR_BUTTON = 0,
    PR_AXIS_POSITIVE = 1,
    PR_AXIS_NEGATIVE = 2,
};
struct GamepadBinding {
    int bind_index;
    GamepadBindingType type;
};

struct KeyboardBinding {
    int bind_index;
};

struct InputAction {
    KeyboardBinding kb_binds[2];
    GamepadBinding gp_binds[2];

    Key key;
    // Analog value of the action, always between 0 and 1
    float value;
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
    InputAction actions[PR_LAST_ACTION+1];

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

void input_controller_init(InputController *input);

#endif
