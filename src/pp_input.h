#ifndef PP_INPUT_H
#define PP_INPUT_H

#include "../include/glfw3.h"

struct InputController {
    // Gameplay
    // TODO: This is mapped to the vertical movement
    //       of the left joystick of the controller,
    //       maybe is better to do everything with the
    //       horizontal movement?
    //       Another solution could be:
    //        - Vertical joystick movement for the plane
    //        - Horizontal joystick movement for the rider
    float left_right;
    bool boost;
    bool reset;

    bool jump;

    // Debug
    bool toggle_debug;
};

/* Updates the InputController global struct
 *
 */
void input_controller_update(GLFWwindow* window, InputController* input);

#endif
