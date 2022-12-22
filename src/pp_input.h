#ifndef PP_INPUT_H
#define PP_INPUT_H

#include "../include/glfw3.h"

struct InputController {
    // Gameplay
    bool move_up;
    bool move_down;
    float vertical;
    bool boost;
    bool reset;

    // Debug
    bool toggle_debug;
};

/* Updates the InputController global struct
 *
 */
void input_controller_update(GLFWwindow* window, InputController* input);

#endif
