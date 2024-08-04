#ifndef PP_INPUT_H
#define PP_INPUT_H

#include "../include/glfw3.h"
#include "../include/glm/vec2.hpp"

#define ACTION_PRESSED(action) (input->actions[(action)].key.pressed)
#define ACTION_CLICKED(action) (input->actions[(action)].key.clicked)
#define ACTION_VALUE(action) (input->actions[(action)].value)

// TODO: Put this as an option_slider to be modified in the keybindings pane
#define PR_GAMEPAD_DEADZONE (0.5f)

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

#define PR_PLAY_PAUSE 16
#define PR_PLAY_RESUME 17
#define PR_PLAY_RESTART 18
#define PR_PLAY_QUIT 19

// Editing actions
#define PR_EDIT_TOGGLE_MODE 20
#define PR_EDIT_SAVE_MAP 21
#define PR_EDIT_OBJ_DELETE 22
#define PR_EDIT_OBJ_CREATE 23
#define PR_EDIT_OBJ_DUPLICATE 24
#define PR_EDIT_PLANE_RESET 25
#define PR_EDIT_MOVE_UP 26
#define PR_EDIT_MOVE_DOWN 27
#define PR_EDIT_MOVE_LEFT 28
#define PR_EDIT_MOVE_RIGHT 29
#define PR_EDIT_OBJ_PROPERTIES_SHOW_TOGGLE 30
#define PR_EDIT_OBJ_PROPERTY_LEFT 31
#define PR_EDIT_OBJ_PROPERTY_RIGHT 32
#define PR_EDIT_OBJ_PROPERTY_TOGGLE 33
#define PR_EDIT_OBJ_PROPERTY_NEXT_INCREMENT 34
#define PR_EDIT_OBJ_PROPERTY_INCREASE 35
#define PR_EDIT_OBJ_PROPERTY_DECREASE 36
#define PR_EDIT_OBJ_SELECTION_UP 37 // Up, but in the screen area
#define PR_EDIT_OBJ_SELECTION_DOWN 38 // Down, but in the screen area
#define PR_EDIT_OBJ_SELECTION_LEFT 39 // Nearest object on the left, anywhere
#define PR_EDIT_OBJ_SELECTION_RIGHT 40 // Nearest object on the right, anywhere
#define PR_EDIT_OBJ_DESELECT 41

#define PR_LAST_ACTION PR_EDIT_OBJ_DESELECT

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

struct InputController {
    // Check if the bindings were modified, and put to false when the change
    // has been noticed
    bool modified;

    InputAction actions[PR_LAST_ACTION+1];

    // Binding we are modifying
    KeyboardBinding *kb_binding;
    GamepadBinding *gp_binding;

    // Binding we just modified,
    // so they are disabled until they are not pressed anymore
    KeyboardBinding kb_disabled_until_released;
    GamepadBinding gp_disabled_until_released;

    // Gameplay
    int8_t current_gamepad;
    char *gamepad_name;

    // NOTE: Mouse
    Key mouse_left;
    Key mouse_right;
    Key mouse_middle;
    double mouseX;
    double mouseY;
    double old_mouseX;
    double old_mouseY;
    bool was_mouse_moved;
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

void
input_controller_init(InputController *input);

/* Updates the InputController global struct */
void
input_controller_update(GLFWwindow *window, InputController *input, const int vertical_bar, const int horizontal_bar, const int screen_w, const int screen_h);

void
kb_change_binding_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

const char *get_gamepad_button_name(int key, GamepadBindingType type);
const char *get_key_name(int key);
const char *get_action_name(int action);

// Save and load keybindings from memory
int
keybindings_save_to_file(const char *file_path, InputAction *actions, int actions_len);
int
keybindings_load_from_file(const char *file_path, InputAction *actions, int actions_len);

#endif
