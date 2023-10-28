#include "pp_input.h"

#include "pp_globals.h"
#include <iostream>

#define IS_KEY_PRESSED(key) (glfwGetKey(window, key) == GLFW_PRESS)

#define GP_NO_BINDING { GLFW_KEY_UNKNOWN, PR_BUTTON }
#define KB_NO_BINDING { GLFW_KEY_UNKNOWN }

void input_controller_init(InputController *input) {
    // Detect already connected controllers at startup
    input->current_gamepad = -1;
    input->gamepad_name = NULL;
    for(size_t jid = 0; jid < GLFW_JOYSTICK_LAST; ++jid) {
        if (glfwJoystickIsGamepad(jid)) {
            input->current_gamepad = jid;
            input->gamepad_name = (char *)glfwGetGamepadName(jid);
            break;
        }
    }

    InputAction *actions = input->actions;

    // Global actions
    actions[PR_EXIT_GAME] = {
        .kb_binds = { { GLFW_KEY_F4 }, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING }
    };

    // Menu actions
    actions[PR_MENU_UP] = {
        .kb_binds = { { GLFW_KEY_UP }, { GLFW_KEY_W } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_UP, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_Y, PR_AXIS_POSITIVE } }
    };
    actions[PR_MENU_DOWN] = {
        .kb_binds = { { GLFW_KEY_DOWN }, { GLFW_KEY_S } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_DOWN, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_Y, PR_AXIS_NEGATIVE } }
    };
    actions[PR_MENU_LEFT] = {
        .kb_binds = { { GLFW_KEY_LEFT }, { GLFW_KEY_A } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_LEFT, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_X, PR_AXIS_NEGATIVE } }
    };
    actions[PR_MENU_RIGHT] = {
        .kb_binds = { { GLFW_KEY_RIGHT }, { GLFW_KEY_D } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_X, PR_AXIS_POSITIVE } }
    };
    actions[PR_MENU_CLICK] = {
        .kb_binds = { { GLFW_KEY_ENTER }, { GLFW_KEY_SPACE } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_CROSS, PR_BUTTON },
                      GP_NO_BINDING }
    };
    actions[PR_MENU_LEVEL_DELETE] = {
        .kb_binds = { { GLFW_KEY_C }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_TRIANGLE, PR_BUTTON },
                      GP_NO_BINDING }
    };
    actions[PR_MENU_LEVEL_EDIT] = {
        .kb_binds = { { GLFW_KEY_E }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_SQUARE, PR_BUTTON},
                      GP_NO_BINDING }
    };
    actions[PR_MENU_PANE_RIGHT] = {
        .kb_binds = { { GLFW_KEY_X }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, PR_BUTTON},
                      GP_NO_BINDING }
    };
    actions[PR_MENU_PANE_RIGHT] = {
        .kb_binds = { { GLFW_KEY_Z }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, PR_BUTTON},
                      GP_NO_BINDING }
    };
    actions[PR_MENU_EXIT] = {
        .kb_binds = { { GLFW_KEY_ESCAPE }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_CIRCLE, PR_BUTTON},
                      GP_NO_BINDING }
    };

    // Gameplay actions
    actions[PR_PLAY_PLANE_UP] = {
        .kb_binds = { { GLFW_KEY_A }, { GLFW_KEY_LEFT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_LEFT_Y, PR_AXIS_POSITIVE},
                      { GLFW_GAMEPAD_BUTTON_DPAD_UP, PR_BUTTON } }
    };
    actions[PR_PLAY_PLANE_DOWN] = {
        .kb_binds = { { GLFW_KEY_D }, { GLFW_KEY_RIGHT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_LEFT_Y, PR_AXIS_NEGATIVE },
                      { GLFW_GAMEPAD_BUTTON_DPAD_DOWN, PR_BUTTON } }
    };
    actions[PR_PLAY_RIDER_RIGHT] = {
        .kb_binds = { { GLFW_KEY_D }, { GLFW_KEY_RIGHT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_RIGHT_X, PR_AXIS_POSITIVE },
                      { GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, PR_BUTTON } }
    };
    actions[PR_PLAY_RIDER_LEFT] = {
        .kb_binds = { { GLFW_KEY_A }, { GLFW_KEY_LEFT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_RIGHT_X, PR_AXIS_NEGATIVE },
                      { GLFW_GAMEPAD_BUTTON_DPAD_LEFT, PR_BUTTON } }
    };
    actions[PR_PLAY_RIDER_JUMP] = {
        .kb_binds = { { GLFW_KEY_SPACE }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, PR_BUTTON },
                      { GLFW_GAMEPAD_BUTTON_CROSS, PR_BUTTON } }
    };

    // TODO: Improve editing keybindings
    // Editing actions
    actions[PR_EDIT_TOGGLE] = {
        .kb_binds = { { GLFW_KEY_E }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_LEFT_THUMB, PR_BUTTON },
                      GP_NO_BINDING }
    };
    actions[PR_EDIT_SAVE_MAP] = {
        .kb_binds = { { GLFW_KEY_M }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, PR_BUTTON },
                      GP_NO_BINDING }
    };
    actions[PR_EDIT_OBJ_DELETE] = {
        .kb_binds = { { GLFW_KEY_K }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_SQUARE, PR_BUTTON },
                      GP_NO_BINDING }
    };
    actions[PR_EDIT_OBJ_CREATE] = {
        .kb_binds = { { GLFW_KEY_N }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_CIRCLE, PR_BUTTON },
                      GP_NO_BINDING }
    };
    actions[PR_EDIT_OBJ_DUPLICATE] = {
        .kb_binds = { { GLFW_KEY_L }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_TRIANGLE, PR_BUTTON } }
    };
    // TODO: Complete
}

void input_controller_update(GLFWwindow *window, InputController *input,
                             const int vertical_bar,
                             const int horizontal_bar,
                             const int screen_w,
                             const int screen_h) {
    // NOTE: Multiple values might be set based off the same keys.
    //       This could happen because a menu action has the
    //          same keybinding as a gameplay one.

    // ### NEW RESET ###
    for(size_t action_index = 0;
        action_index < ARRAY_LENGTH(input->actions);
        ++action_index) {

        InputAction *action = &input->actions[action_index];

        key_reset(&action->key);
        action->value = 0.f;
    }


    // ### OLD RESET ###
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
    key_reset(&input->menu_custom_delete);
    key_reset(&input->menu_custom_edit);
    key_reset(&input->menu_pane_right);
    key_reset(&input->menu_pane_left);
    key_reset(&input->menu_to_start_menu);

    // Debug
    key_reset(&input->debug);

    key_reset(&input->up);
    key_reset(&input->down);
    key_reset(&input->left);
    key_reset(&input->right);

    // ### OLD STUFF ###

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
        glfwGetCursorPos(window, &input->mouseX, &input->mouseY);
        input->mouseX -= vertical_bar;
        input->mouseX *= (float)GAME_WIDTH / (float)screen_w;
        input->mouseY -= horizontal_bar;
        input->mouseY *= (float)GAME_HEIGHT / (float)screen_h;
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


    bool wasGamepadFound = false;
    GLFWgamepadstate gamepad;

    if (input->current_gamepad >= 0 &&
            glfwGetGamepadState(input->current_gamepad, &gamepad)) {
        wasGamepadFound = true;
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_CROSS] == GLFW_PRESS) {
            key_pressed(&input->menu_click);
            key_pressed(&input->jump);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS) {
            key_pressed(&input->jump);
            key_pressed(&input->menu_pane_left);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS) {
            key_pressed(&input->menu_pane_right);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_SQUARE] == GLFW_PRESS) {
            key_pressed(&input->menu_custom_edit);
            key_pressed(&input->restart);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_CIRCLE] == GLFW_PRESS) {
            key_pressed(&input->quit);
            key_pressed(&input->menu_to_start_menu);
        }
        if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_TRIANGLE] == GLFW_PRESS) {
            key_pressed(&input->menu_custom_delete);
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
        if (glm::abs(gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]) > 0.3f) {
            input->up_down = gamepad.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
        }
        if (glm::abs(gamepad.axes[GLFW_GAMEPAD_AXIS_LEFT_X]) > 0.3f) {
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
    if(IS_KEY_PRESSED(GLFW_KEY_Q)) {
        key_pressed(&input->menu_to_start_menu);
    }

    // ### NEW STUFF ###
    for(size_t action_index = 0;
        action_index < ARRAY_LENGTH(input->actions);
        ++action_index) {

        InputAction *action = &input->actions[action_index];

        for(size_t kb_bind_index = 0;
            kb_bind_index < ARRAY_LENGTH(action->kb_binds);
            ++kb_bind_index) {

            int bind_index = action->kb_binds[kb_bind_index].bind_index;

            if (bind_index == GLFW_KEY_UNKNOWN) continue;

            if (bind_index >= 0 && IS_KEY_PRESSED(bind_index)) {

                key_pressed(&action->key);
                action->value = 1.f;
            }
        }

        // Check for gamepad input only if the gamepad is present
        if (!wasGamepadFound) continue;

        for(size_t gp_bind_index = 0;
            gp_bind_index < ARRAY_LENGTH(action->gp_binds);
            ++gp_bind_index) {

            GamepadBindingType type = action->gp_binds[gp_bind_index].type;
            int bind_index = action->gp_binds[gp_bind_index].bind_index;

            if (bind_index == GLFW_KEY_UNKNOWN) continue;

            if (type == PR_BUTTON) {
                if (gamepad.buttons[bind_index] == GLFW_PRESS) {
                    key_pressed(&action->key);
                    action->value = 1.f;
                }
            } else if (type == PR_AXIS_POSITIVE) {
                if (gamepad.axes[bind_index] > PR_GAMEPAD_DEADZONE) {
                    key_pressed(&action->key);
                    action->value = gamepad.axes[bind_index];
                }
            } else if (type == PR_AXIS_NEGATIVE) {
                if (gamepad.axes[bind_index] < -PR_GAMEPAD_DEADZONE) {
                    key_pressed(&action->key);
                    // I always want to have a value in [0,1] and
                    // that's why this value is negated
                    action->value = -gamepad.axes[bind_index];
                }
            }
        }
    }
}
