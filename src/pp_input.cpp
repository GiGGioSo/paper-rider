#include "pp_input.h"

#include "pp_globals.h"
#include <iostream>

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
    // Menu
    key_reset(&input->level1);
    key_reset(&input->level2);
    // Debug
    key_reset(&input->debug);

    key_reset(&input->up);
    key_reset(&input->down);
    key_reset(&input->left);
    key_reset(&input->right);

    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        key_pressed(&input->up);
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        key_pressed(&input->down);
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        key_pressed(&input->left);
    }
    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        key_pressed(&input->right);
    }

    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
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
            // std::cout << "[WARNING] Controller with "
            //           << axes_count << " axes!!"
            //           << std::endl;
            axes = NULL;
        }

        int buttons_count;
        buttons =
            (unsigned char *) glfwGetJoystickButtons(input->current_joystick,
                                                     &buttons_count);
        // TODO: Decent Warning handling
        if (buttons_count != 17) {
            // std::cout << "[WARNING] Controller with "
            //           << buttons_count << " buttons!!"
            //           << std::endl;
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
    // TODO: Handle abnormal gamepads

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        key_pressed(&input->level1);
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        key_pressed(&input->level2);
    }
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        key_pressed(&input->menu);
    }


    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        input->left_right = -1.0f;

    } else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        input->left_right = 1.0f;

    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        key_pressed(&input->debug);
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        key_pressed(&input->jump);
    }

    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        key_pressed(&input->boost);
    }
}
