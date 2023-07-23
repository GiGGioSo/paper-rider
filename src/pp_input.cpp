#include "pp_input.h"

#include "pp_globals.h"
#include <iostream>

#define IS_KEY_PRESSED(key) (glfwGetKey(window, key) == GLFW_PRESS)

void input_controller_update(GLFWwindow *window, InputController* input,
                             const float screen_w, const float screen_h) {
    // NOTE: Multiple values might be set based off the same keys.
    //       This could happen because a menu action has the
    //          same keybinding as a gameplay one.

    // reset all to default
    // Global
    key_reset(&input->exit);

    // Mouse
    key_reset(&input->mouse_left);
    key_reset(&input->mouse_right);
    key_reset(&input->mouse_middle);
    input->old_mouseX = input->mouseX;
    input->old_mouseY = input->mouseY;
    input->mouseX = 0.0;
    input->mouseY = 0.0;
    input->was_mouse_moved = false;

    // Gameplay
    key_reset(&input->boost);
    input->up_down = 0.f;
    input->left_right = 0.f;

    key_reset(&input->jump);

    key_reset(&input->pause);
    key_reset(&input->resume);
    key_reset(&input->restart);
    key_reset(&input->quit);
    // Editing
    key_reset(&input->edit);
    key_reset(&input->save_map);
    key_reset(&input->obj_delete);
    key_reset(&input->obj_add);
    key_reset(&input->obj_duplicate);
    key_reset(&input->reset_pos);

    key_reset(&input->menu_up);
    key_reset(&input->menu_down);
    key_reset(&input->menu_left);
    key_reset(&input->menu_right);
    key_reset(&input->menu_click);
    key_reset(&input->menu_to_custom);
    key_reset(&input->menu_to_campaign);

    // Debug
    key_reset(&input->debug);

    key_reset(&input->up);
    key_reset(&input->down);
    key_reset(&input->left);
    key_reset(&input->right);

    if(IS_KEY_PRESSED(GLFW_KEY_UP)) {
        key_pressed(&input->up);
        key_pressed(&input->menu_up);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_DOWN)) {
        key_pressed(&input->down);
        key_pressed(&input->menu_down);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_LEFT)) {
        key_pressed(&input->left);
        key_pressed(&input->menu_left);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_RIGHT)) {
        key_pressed(&input->right);
        key_pressed(&input->menu_right);
    }

    if(IS_KEY_PRESSED(GLFW_KEY_F4)) {
        key_pressed(&input->exit);
    }

    // Mouse fetching
    if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_HIDDEN) {
        glfwGetCursorPos(window,
                         &input->mouseX,
                         &input->mouseY);
        input->mouseX *= GAME_WIDTH / screen_w;
        input->mouseY *= GAME_HEIGHT / screen_h;
        if (input->mouseX != input->old_mouseX ||
            input->mouseY != input->old_mouseY) {
            input->was_mouse_moved = true;
        }
        // TODO: Maybe clip it to screen coordinates
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            key_pressed(&input->mouse_left);
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            key_pressed(&input->mouse_right);
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            key_pressed(&input->mouse_middle);
        }
    }

    GLFWgamepadstate gamepad;

    if (input->current_gamepad >= 0 &&
            glfwGetGamepadState(input->current_gamepad, &gamepad)) {
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_CROSS] == GLFW_PRESS) {
            key_pressed(&input->menu_click);
            key_pressed(&input->jump);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS) {
            key_pressed(&input->jump);
            key_pressed(&input->menu_to_campaign);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS) {
            key_pressed(&input->menu_to_custom);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_SQUARE] == GLFW_PRESS) {
            key_pressed(&input->restart);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_CIRCLE] == GLFW_PRESS) {
            key_pressed(&input->quit);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_TRIANGLE] == GLFW_PRESS) {
            key_pressed(&input->pause);
            key_pressed(&input->resume);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS) {
            key_pressed(&input->up);
            key_pressed(&input->menu_up);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS) {
            key_pressed(&input->down);
            key_pressed(&input->menu_down);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS) {
            key_pressed(&input->left);
            key_pressed(&input->menu_left);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS) {
            key_pressed(&input->right);
            key_pressed(&input->menu_right);
        }
        if (glm::abs(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]) > 0.1f) {
            input->up_down = gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
        }
        if (glm::abs(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X]) > 0.1f) {
            input->left_right = gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        }
    }

    if (IS_KEY_PRESSED(GLFW_KEY_ESCAPE)) {
        key_pressed(&input->pause);
        key_pressed(&input->resume);
    }
    if (IS_KEY_PRESSED(GLFW_KEY_Y)) {
        key_pressed(&input->restart);
    }
    if (IS_KEY_PRESSED(GLFW_KEY_P)) {
        key_pressed(&input->quit);
    }

    // Maybe different keybindings for up_down
    if(IS_KEY_PRESSED(GLFW_KEY_A)) { 
        input->left_right = -1.0f;
        input->up_down = -1.0f;
    } else if(IS_KEY_PRESSED(GLFW_KEY_D)) {
        input->left_right = 1.0f;
        input->up_down = 1.0f;
    }

    if (IS_KEY_PRESSED(GLFW_KEY_J) || IS_KEY_PRESSED(GLFW_KEY_SPACE)) {
        key_pressed(&input->jump);
    }

    // Editing
    if (IS_KEY_PRESSED(GLFW_KEY_E)) {
        key_pressed(&input->edit);
    }
    if (IS_KEY_PRESSED(GLFW_KEY_M)) {
        key_pressed(&input->save_map);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_C)) {
        key_pressed(&input->obj_delete);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_V)) {
        key_pressed(&input->obj_add);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_B)) {
        key_pressed(&input->obj_duplicate);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_R)) {
        key_pressed(&input->reset_pos);
    }
}
