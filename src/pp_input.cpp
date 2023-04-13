#include "pp_input.h"

#include "pp_globals.h"
#include <iostream>

#define IS_KEY_PRESSED(key) (glfwGetKey(window, key) == GLFW_PRESS)

void input_controller_update(GLFWwindow *window, InputController* input) {
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
    input->mouseX = 0.0;
    input->mouseY = 0.0;

    // Gameplay
    key_reset(&input->boost);
    input->left_right = 0.f;

    key_reset(&input->jump);

    key_reset(&input->menu);
    // Editing
    key_reset(&input->edit);
    key_reset(&input->save_map);
    key_reset(&input->obj_delete);
    key_reset(&input->obj_add);
    key_reset(&input->reset_pos);

    // Menu
    key_reset(&input->level1);
    key_reset(&input->level2);
    // Debug
    key_reset(&input->debug);

    key_reset(&input->up);
    key_reset(&input->down);
    key_reset(&input->left);
    key_reset(&input->right);

    if(IS_KEY_PRESSED(GLFW_KEY_UP)) {
        key_pressed(&input->up);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_DOWN)) {
        key_pressed(&input->down);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_LEFT)) {
        key_pressed(&input->left);
    }
    if(IS_KEY_PRESSED(GLFW_KEY_RIGHT)) {
        key_pressed(&input->right);
    }

    if(IS_KEY_PRESSED(GLFW_KEY_ESCAPE)) {
        key_pressed(&input->exit);
    }

    // Mouse fetching
    if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_HIDDEN) {
        glfwGetCursorPos(window,
                         &input->mouseX,
                         &input->mouseY);
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

    float *axes = NULL;
    unsigned char *buttons = NULL;

    if (input->current_joystick >= 0) {
        int axes_count;
        axes = (float *) glfwGetJoystickAxes(input->current_joystick,
                                             &axes_count);
        // TODO: Decent Warning handling
        if (axes_count != 6) {
            std::cout << "[WARNING] Controller with "
                      << axes_count << " axes!!"
                      << std::endl;
            axes = NULL;
        }

        int buttons_count;
        buttons =
            (unsigned char *) glfwGetJoystickButtons(input->current_joystick,
                                                     &buttons_count);
        // TODO: Decent Warning handling
        if (buttons_count != 18) {
            std::cout << "[WARNING] Controller with "
                      << buttons_count << " buttons!!"
                      << std::endl;
            buttons = NULL;
        }
    }

    // TODO: Actually add gamepad input.
    //       
    //       ALWAYS CHECK IF `axes` and `buttons` are NULL before using them,
    //          because if they are NULL then it means there's something
    //          wrong with the gamepad.
    UNUSED(axes);
    UNUSED(buttons);

    if (axes && buttons) {
        if (buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS) {
            key_pressed(&input->jump);
        }
        if (buttons[GLFW_GAMEPAD_BUTTON_X] == GLFW_PRESS) {
            key_pressed(&input->boost);
        }
        if (buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS) {
            key_pressed(&input->menu);
        }
        if (glm::abs(axes[GLFW_GAMEPAD_AXIS_LEFT_Y]) > 0.1f) {
            input->left_right = axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        }
    }


    // TODO: Handle abnormal gamepads

    if (IS_KEY_PRESSED(GLFW_KEY_1)) {
        key_pressed(&input->level1);
    }
    if (IS_KEY_PRESSED(GLFW_KEY_2)) {
        key_pressed(&input->level2);
    }
    if (IS_KEY_PRESSED(GLFW_KEY_0)) {
        key_pressed(&input->menu);
    }


    if(IS_KEY_PRESSED(GLFW_KEY_A)) {
        input->left_right = -1.0f;
    } else if(IS_KEY_PRESSED(GLFW_KEY_D)) {
        input->left_right = 1.0f;
    }

    if (IS_KEY_PRESSED(GLFW_KEY_P)) {
        key_pressed(&input->debug);
    }

    if (IS_KEY_PRESSED(GLFW_KEY_F)) {
        key_pressed(&input->jump);
    }

    if (IS_KEY_PRESSED(GLFW_KEY_SPACE)) {
        key_pressed(&input->boost);
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
    if(IS_KEY_PRESSED(GLFW_KEY_R)) {
        key_pressed(&input->reset_pos);
    }
}
