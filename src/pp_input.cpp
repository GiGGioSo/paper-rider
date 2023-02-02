#include "pp_input.h"

#include "pp_globals.h"
#include <iostream>

void input_controller_update(GLFWwindow *window, InputController* input) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // reset all to false
    input->reset = false;
    input->boost = false;
    input->toggle_debug = false;
    input->left_right = 0.f;
    input->jump = false;

    // Fetching controller input
    // ANALOGICO
    /* float lj_h; // left-joysticj horizontal  -1=left   1=right */
    float lj_v = 0.f; // left-joystick vertical    -1=up     1=down
    /* float rj_h; // right-joysticj horizontal  -1=left   1=right */
    /* float rj_v; // right-joystick vertical    -1=up     1=down */
    /* float lt_an; // analogical left-trigger   -1=released     1=pressed */
    /* float rt_an; // analogical right-trigger   -1=released     1=pressed */

    //DIGITALE
    bool dpad_up = false; // DPAD controls
    /* bool dpad_right; */
    /* bool dpad_left; */
    bool dpad_down = false;

    bool b_up = false; // Button controls (I put the direction instead of the name)
    /* bool b_right; */
    /* bool b_down; */
    /* bool b_left; */

    /* bool ls; // left shoulder */
    /* bool rs; // right shoulder */

    /* bool lt_dig; // digital left trigger */
    /* bool rt_dig; // digital right trigger */

    /* bool lj_click; // right-joystick click */
    /* bool rj_click; // left-joystick click */

    /* bool b_share; // share button */
    /* bool b_opt; // option button */
    /* bool b_logo; // logo button */

    int c1_present = glfwJoystickPresent(GLFW_JOYSTICK_1);
    if (!c1_present);
    else {
        int axes_count;
        const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
        // TODO: Decent Warning handling
        if (axes_count != 6); // std::cout << "[WARNING] Controller with " << axes_count << " axes!!" << std::endl;
        else {
            /* lj_h = axes[0]; */
            lj_v = axes[1];
            /* lt_an = axes[2]; */
            /* rj_h = axes[3]; */
            /* rj_v = axes[4]; */
            /* rt_an = axes[5]; */
        }
        int buttons_count;
        const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
        // TODO: Decent Warning handling
        if (buttons_count != 17); //std::cout << "[WARNING] Controller with " << buttons_count << " buttons!!" << std::endl;
        else {
            /* b_down = buttons[0] == GLFW_PRESS; */
            /* b_right = buttons[1] == GLFW_PRESS; */
            /* b_left = buttons[3] == GLFW_PRESS; */
            b_up = buttons[2] == GLFW_PRESS;
            /* ls = buttons[4] == GLFW_PRESS; */
            /* rs = buttons[5] == GLFW_PRESS; */
            /* lt_dig = buttons[6] == GLFW_PRESS; */
            /* rt_dig = buttons[7] == GLFW_PRESS; */
            /* b_share = buttons[8] == GLFW_PRESS; */
            /* b_opt = buttons[9] == GLFW_PRESS; */
            /* b_logo = buttons[10] == GLFW_PRESS; */
            /* lj_click = buttons[11] == GLFW_PRESS; */
            /* rj_click = buttons[12] == GLFW_PRESS; */
            dpad_up = buttons[13] == GLFW_PRESS;
            /* dpad_right = buttons[14] == GLFW_PRESS; */
            /* dpad_left = buttons[16] == GLFW_PRESS; */
            dpad_down = buttons[15] == GLFW_PRESS;
        }
    }

    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            (c1_present && (lj_v <= -0.2f || dpad_up))) {

        input->left_right = (lj_v <= -0.2f) ? lj_v : -1.0f;

    } else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
            (c1_present && (lj_v >= 0.2f || dpad_down))) {

        input->left_right = (lj_v >= 0.2f) ? lj_v : 1.0f;

    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        input->toggle_debug = true;
    }

    if (glfwGetKey(window, GLFW_KEY_H)) {
        input->jump = true;
    }

    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
        (c1_present && b_up)) {
        input->boost = true;
    }

}
