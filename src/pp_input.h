#ifndef PP_INPUT_H
#define PP_INPUT_H

#include "../include/glfw3.h"
#include "../include/glm/vec2.hpp"

struct Key {
    bool old;
    bool clicked;
    bool pressed;
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
    // Gameplay
    // TODO: This is mapped to the vertical movement
    //       of the left joystick of the controller,
    //       maybe is better to do everything with the
    //       horizontal movement?
    //       Another solution could be:
    //        - Vertical joystick movement for the plane
    //        - Horizontal joystick movement for the rider

    int8_t current_joystick;

    // NOTE: Global
    Key exit;

    // NOTE: Mouse
    Key mouse_left;
    Key mouse_right;
    Key mouse_middle;
    double mouseX;
    double mouseY;

    // NOTE: Gameplay
    float left_right;
    Key boost;
    Key jump;
    Key menu;

    // NOTE: Editing
    Key edit;
    Key save_map;

    // NOTE: Menu
    Key level1;
    Key level2;

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
void input_controller_update(GLFWwindow* window, InputController* input);

#endif
