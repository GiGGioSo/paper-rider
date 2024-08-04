#include "pr_input.h"

#include "pr_globals.h"
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

    input->kb_disabled_until_released = KB_NO_BINDING;
    input->gp_disabled_until_released = GP_NO_BINDING;

    InputAction *actions = input->actions;

    // Global actions
    actions[PR_EXIT_GAME] = {
        .kb_binds = { { GLFW_KEY_F4 }, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };

    // Menu actions
    actions[PR_MENU_UP] = {
        .kb_binds = { { GLFW_KEY_UP }, { GLFW_KEY_W } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_UP, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_Y, PR_AXIS_NEGATIVE } },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_DOWN] = {
        .kb_binds = { { GLFW_KEY_DOWN }, { GLFW_KEY_S } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_DOWN, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_Y, PR_AXIS_POSITIVE } },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_LEFT] = {
        .kb_binds = { { GLFW_KEY_LEFT }, { GLFW_KEY_A } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_LEFT, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_X, PR_AXIS_NEGATIVE } },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_RIGHT] = {
        .kb_binds = { { GLFW_KEY_RIGHT }, { GLFW_KEY_D } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, PR_BUTTON },
                      { GLFW_GAMEPAD_AXIS_LEFT_X, PR_AXIS_POSITIVE } },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_CLICK] = {
        .kb_binds = { { GLFW_KEY_ENTER }, { GLFW_KEY_SPACE } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_CROSS, PR_BUTTON },
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_LEVEL_DELETE] = {
        .kb_binds = { { GLFW_KEY_C }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_TRIANGLE, PR_BUTTON },
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_LEVEL_EDIT] = {
        .kb_binds = { { GLFW_KEY_E }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_SQUARE, PR_BUTTON},
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_PANE_RIGHT] = {
        .kb_binds = { { GLFW_KEY_X }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, PR_BUTTON},
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_PANE_LEFT] = {
        .kb_binds = { { GLFW_KEY_Z }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, PR_BUTTON},
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_MENU_EXIT] = {
        .kb_binds = { { GLFW_KEY_ESCAPE }, { GLFW_KEY_Q } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_CIRCLE, PR_BUTTON},
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };

    // Gameplay actions
    actions[PR_PLAY_PLANE_UP] = {
        .kb_binds = { { GLFW_KEY_A }, { GLFW_KEY_LEFT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_RIGHT_Y, PR_AXIS_NEGATIVE},
                      { GLFW_GAMEPAD_BUTTON_DPAD_UP, PR_BUTTON } },
        .key = {},
        .value = 0.f
    };
    actions[PR_PLAY_PLANE_DOWN] = {
        .kb_binds = { { GLFW_KEY_D }, { GLFW_KEY_RIGHT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_RIGHT_Y, PR_AXIS_POSITIVE },
                      { GLFW_GAMEPAD_BUTTON_DPAD_DOWN, PR_BUTTON } },
        .key = {},
        .value = 0.f
    };
    actions[PR_PLAY_RIDER_RIGHT] = {
        .kb_binds = { { GLFW_KEY_D }, { GLFW_KEY_RIGHT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_LEFT_X, PR_AXIS_POSITIVE },
                      { GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, PR_BUTTON } },
        .key = {},
        .value = 0.f
    };
    actions[PR_PLAY_RIDER_LEFT] = {
        .kb_binds = { { GLFW_KEY_A }, { GLFW_KEY_LEFT } },
        .gp_binds = { { GLFW_GAMEPAD_AXIS_LEFT_X, PR_AXIS_NEGATIVE },
                      { GLFW_GAMEPAD_BUTTON_DPAD_LEFT, PR_BUTTON } },
        .key = {},
        .value = 0.f
    };
    actions[PR_PLAY_RIDER_JUMP] = {
        .kb_binds = { { GLFW_KEY_SPACE }, { GLFW_KEY_J } },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, PR_BUTTON },
                      { GLFW_GAMEPAD_BUTTON_CROSS, PR_BUTTON } },
        .key = {},
        .value = 0.f
    };

    actions[PR_PLAY_PAUSE] = {
        .kb_binds = { { GLFW_KEY_ESCAPE }, KB_NO_BINDING },
        .gp_binds = {{GLFW_GAMEPAD_BUTTON_TRIANGLE, PR_BUTTON}, GP_NO_BINDING},
        .key = {},
        .value = 0.f
    };
    actions[PR_PLAY_RESUME] = {
        .kb_binds = { { GLFW_KEY_ESCAPE }, KB_NO_BINDING },
        .gp_binds = {{GLFW_GAMEPAD_BUTTON_TRIANGLE, PR_BUTTON}, GP_NO_BINDING},
        .key = {},
        .value = 0.f
    };
    actions[PR_PLAY_RESTART] = {
        .kb_binds = { { GLFW_KEY_Y }, KB_NO_BINDING },
        .gp_binds = { {GLFW_GAMEPAD_BUTTON_SQUARE, PR_BUTTON}, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_PLAY_QUIT] = {
        .kb_binds = { { GLFW_KEY_P }, KB_NO_BINDING },
        .gp_binds = { {GLFW_GAMEPAD_BUTTON_CIRCLE, PR_BUTTON}, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };

    // TODO: Improve editing keybindings
    // Editing actions
    actions[PR_EDIT_TOGGLE_MODE] = {
        .kb_binds = { { GLFW_KEY_E }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_LEFT_THUMB, PR_BUTTON },
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_SAVE_MAP] = {
        .kb_binds = { { GLFW_KEY_M }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_BACK, PR_BUTTON },
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_DELETE] = {
        .kb_binds = { { GLFW_KEY_K }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_SQUARE, PR_BUTTON },
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_CREATE] = {
        .kb_binds = { { GLFW_KEY_N }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_CIRCLE, PR_BUTTON },
                      GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_DUPLICATE] = {
        .kb_binds = { { GLFW_KEY_L }, KB_NO_BINDING },
        .gp_binds = { { GLFW_GAMEPAD_BUTTON_TRIANGLE, PR_BUTTON } },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_PLANE_RESET] = {
        .kb_binds = { { GLFW_KEY_R }, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_MOVE_UP] = {
        .kb_binds = { { GLFW_KEY_W }, { GLFW_KEY_UP } },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_MOVE_DOWN] = {
        .kb_binds = { { GLFW_KEY_S }, { GLFW_KEY_DOWN } },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_MOVE_LEFT] = {
        .kb_binds = { { GLFW_KEY_A }, { GLFW_KEY_LEFT } },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_MOVE_RIGHT] = {
        .kb_binds = { { GLFW_KEY_D }, { GLFW_KEY_RIGHT } },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_PROPERTIES_SHOW_TOGGLE] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_PROPERTY_LEFT] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_PROPERTY_RIGHT] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_PROPERTY_TOGGLE] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_PROPERTY_NEXT_INCREMENT] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_PROPERTY_INCREASE] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_PROPERTY_DECREASE] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_SELECTION_UP] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_SELECTION_DOWN] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_SELECTION_LEFT] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_SELECTION_RIGHT] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    actions[PR_EDIT_OBJ_DESELECT] = {
        .kb_binds = { KB_NO_BINDING, KB_NO_BINDING },
        .gp_binds = { GP_NO_BINDING, GP_NO_BINDING },
        .key = {},
        .value = 0.f
    };
    // TODO: Complete

    // Load keybindings from file, and overwrite the default ones
    int keybinds_load_ret = keybindings_load_from_file("./my_binds.prkeys",
                               input->actions, ARRAY_LENGTH(input->actions));
    if (keybinds_load_ret) {
        std::cout << "Could not load keybindings from file ./my_binds.prkeys: "
                  << keybinds_load_ret
                  << std::endl;
    } else {
        std::cout << "Loaded keybindings from file ./my_binds.prkeys"
                  << std::endl;
    }
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
    // Mouse resetting
    key_reset(&input->mouse_left);
    key_reset(&input->mouse_right);
    key_reset(&input->mouse_middle);
    input->old_mouseX = input->mouseX;
    input->old_mouseY = input->mouseY;
    input->mouseX = 0.0;
    input->mouseY = 0.0;
    input->was_mouse_moved = false;

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
    }

    // If ESC is pressed, stop waiting for a new gamepad binding
    if (input->gp_binding &&
            glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        input->gp_binding = NULL;
        input->kb_disabled_until_released = { GLFW_KEY_ESCAPE };
    }
    // Check for a new gamepad binding
    if (input->gp_binding && wasGamepadFound) {
        for(int button_index = 0;
            button_index <= GLFW_GAMEPAD_BUTTON_LAST && input->gp_binding;
            ++button_index) {
            if (gamepad.buttons[button_index] == GLFW_PRESS) {
                input->gp_binding->bind_index = button_index;
                input->gp_binding->type = PR_BUTTON;
                input->modified = true;
                input->gp_disabled_until_released = *input->gp_binding;
                input->gp_binding = NULL;
            }
        }
        for(int axis_index = 0;
            axis_index <= GLFW_GAMEPAD_AXIS_LAST && input->gp_binding;
            ++axis_index) {

            float axis_value = gamepad.axes[axis_index];

            // Remap the value if it's a trigger
            if (axis_index == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER ||
                    axis_index == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER) {
                axis_value = (axis_value + 1.f) * 0.5f;
            }

            if (axis_value > PR_GAMEPAD_DEADZONE) {
                input->gp_binding->bind_index = axis_index;
                input->gp_binding->type = PR_AXIS_POSITIVE;
                input->modified = true;
                input->gp_disabled_until_released = *input->gp_binding;
                input->gp_binding = NULL;
            } else if (axis_value < -PR_GAMEPAD_DEADZONE) {
                input->gp_binding->bind_index = axis_index;
                input->gp_binding->type = PR_AXIS_NEGATIVE;
                input->modified = true;
                input->gp_disabled_until_released = *input->gp_binding;
                input->gp_binding = NULL;
            }
        }
    }

    // Check if actions are being pressed by either keyboard or gamepad
    for(size_t action_index = 0;
            action_index < ARRAY_LENGTH(input->actions);
            ++action_index) {

        InputAction *action = &input->actions[action_index];

        // NOTE: Update actions based on keyboard only
        //          if there's no change of keybinding going on
        if (input->kb_binding == NULL) {
            for(size_t kb_bind_index = 0;
                    kb_bind_index < ARRAY_LENGTH(action->kb_binds);
                    ++kb_bind_index) {

                int bind_index = action->kb_binds[kb_bind_index].bind_index;

                bool disable_press = false;
                if (input->kb_disabled_until_released.bind_index ==
                        bind_index) {
                    disable_press = true;
                }

                if (bind_index == GLFW_KEY_UNKNOWN) continue;

                if (bind_index >= 0 && IS_KEY_PRESSED(bind_index)) {
                    if (!disable_press) {
                        key_pressed(&action->key);
                        action->value = 1.f;
                    }
                } else if (disable_press) {
                    input->kb_disabled_until_released = KB_NO_BINDING;
                }
            }
        }

        // NOTE: Update actions based on gamepad only
        //          if there's no change of keybindings going on
        //              AND 
        //          if the gamepad was found
        if (input->gp_binding == NULL && wasGamepadFound) {

            for(size_t gp_bind_index = 0;
                    gp_bind_index < ARRAY_LENGTH(action->gp_binds);
                    ++gp_bind_index) {

                if (action->gp_binds[gp_bind_index].bind_index ==
                        GLFW_KEY_UNKNOWN) continue;

                GamepadBindingType type = action->gp_binds[gp_bind_index].type;
                int bind_index = action->gp_binds[gp_bind_index].bind_index;

                bool disable_press = false;
                if (input->gp_disabled_until_released.bind_index ==
                        bind_index &&
                    input->gp_disabled_until_released.type ==
                        type) {
                    disable_press = true;
                }

                if (type == PR_BUTTON) {
                    if (gamepad.buttons[bind_index] == GLFW_PRESS) {
                        if (!disable_press) {
                            key_pressed(&action->key);
                            action->value = 1.f;
                        }
                    } else if (disable_press) {
                        input->gp_disabled_until_released = GP_NO_BINDING;
                    }
                } else if (type == PR_AXIS_POSITIVE) {
                    if (gamepad.axes[bind_index] > PR_GAMEPAD_DEADZONE) {
                        if (!disable_press) {
                            key_pressed(&action->key);
                            action->value = gamepad.axes[bind_index];
                        }
                    } else if (disable_press) {
                        input->gp_disabled_until_released = GP_NO_BINDING;
                    }
                } else if (type == PR_AXIS_NEGATIVE) {
                    if (gamepad.axes[bind_index] < -PR_GAMEPAD_DEADZONE) {
                        if (!disable_press) {
                            key_pressed(&action->key);
                            // I always want to have a value in [0,1] and
                            // that's why this value is negated
                            action->value = -gamepad.axes[bind_index];
                        }
                    } else if (disable_press) {
                        input->gp_disabled_until_released = GP_NO_BINDING;
                    }
                }
            }
        }
    }

}

void kb_change_binding_callback(GLFWwindow *window,
                                int key, int scancode, int action, int mods) {
    UNUSED(scancode);
    UNUSED(mods);

    InputController *input = &glob->input;

    if (!input->kb_binding) return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        input->kb_binding = NULL;
        input->gp_binding = NULL;
        input->kb_disabled_until_released = { GLFW_KEY_ESCAPE };
        glfwSetKeyCallback(window, NULL);
        return;
    }

    if (action == GLFW_PRESS) {
        input->kb_binding->bind_index = key;
        input->modified = true;
        input->kb_disabled_until_released = *input->kb_binding;
        input->kb_binding = NULL;
        glfwSetKeyCallback(window, NULL);
        return;
    }
}

const char *get_gamepad_button_name(int key, GamepadBindingType type) {
    if (type == PR_BUTTON) {
        switch (key) {
            case GLFW_GAMEPAD_BUTTON_A: return "A";
            case GLFW_GAMEPAD_BUTTON_B: return "B";
            case GLFW_GAMEPAD_BUTTON_X: return "X";
            case GLFW_GAMEPAD_BUTTON_Y: return "Y";
            case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: return "LB";
            case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: return "RB";
            case GLFW_GAMEPAD_BUTTON_BACK: return "BACK";
            case GLFW_GAMEPAD_BUTTON_START: return "START";
            case GLFW_GAMEPAD_BUTTON_GUIDE: return "GUIDE";
            case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: return "LSB";
            case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: return "RSB";
            case GLFW_GAMEPAD_BUTTON_DPAD_UP: return "UP";
            case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: return "RIGHT";
            case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: return "DOWN";
            case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: return "LEFT";
            default: return NULL;
        }
    } else if (type == PR_AXIS_POSITIVE) {
        switch (key) {
            case GLFW_GAMEPAD_AXIS_LEFT_X: return "L_RIGHT";
            case GLFW_GAMEPAD_AXIS_LEFT_Y: return "L_DOWN";
            case GLFW_GAMEPAD_AXIS_RIGHT_X: return "R_RIGHT";
            case GLFW_GAMEPAD_AXIS_RIGHT_Y: return "R_DOWN";
            case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER: return "LT";
            case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: return "RT";
            default: return NULL;
        }
    } else if (type == PR_AXIS_NEGATIVE) {
        switch (key) {
            case GLFW_GAMEPAD_AXIS_LEFT_X: return "L_LEFT";
            case GLFW_GAMEPAD_AXIS_LEFT_Y: return "L_UP";
            case GLFW_GAMEPAD_AXIS_RIGHT_X: return "R_LEFT";
            case GLFW_GAMEPAD_AXIS_RIGHT_Y: return "R_UP";
            case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER: return "LT-";
            case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: return "RT-";
            default: return NULL;
        }
    } else {
        return NULL;
    }
}

const char *get_key_name(int key) {
    switch (key) {
        case GLFW_KEY_UP: return "K_UP";
        case GLFW_KEY_DOWN: return "K_DOWN";
        case GLFW_KEY_LEFT: return "K_LEFT";
        case GLFW_KEY_RIGHT: return "K_RIGHT";
        case GLFW_KEY_ENTER: return "ENTER";
        case GLFW_KEY_SPACE: return "SPACE";
        default: return glfwGetKeyName(key, glfwGetKeyScancode(key));
    }
}

const char *get_action_name(int action) {
    if (action < 0 || action > PR_LAST_ACTION) return "INVALID_ACTION";

    switch (action) {
        case PR_EXIT_GAME: return "EXIT GAME";
        case PR_MENU_UP: return "MENU_UP";
        case PR_MENU_DOWN: return "MENU_DOWN";
        case PR_MENU_LEFT: return "MENU_LEFT";
        case PR_MENU_RIGHT: return "MENU_RIGHT";
        case PR_MENU_CLICK: return "MENU_CLICK";
        case PR_MENU_LEVEL_DELETE: return "PR_MENU_LEVEL_DELETE";
        case PR_MENU_LEVEL_EDIT: return "PR_MENU_LEVEL_EDIT";
        case PR_MENU_PANE_RIGHT: return "MENU_PANE_RIGHT";
        case PR_MENU_PANE_LEFT: return "MENU_PANE_LEFT";
        case PR_MENU_EXIT: return "MENU_EXIT";
        case PR_PLAY_PLANE_UP: return "PLAY_PLANE_UP";
        case PR_PLAY_PLANE_DOWN: return "PLAY_PLANE_DOWN";
        case PR_PLAY_RIDER_RIGHT: return "PLAY_RIDER_RIGHT";
        case PR_PLAY_RIDER_LEFT: return "PLAY_RIDER_LEFT";
        case PR_PLAY_RIDER_JUMP: return "PLAY_RIDER_JUMP";
        case PR_PLAY_PAUSE: return "PLAY_PAUSE";
        case PR_PLAY_RESUME: return "PLAY_RESUME";
        case PR_PLAY_RESTART: return "PLAY_RESTART";
        case PR_PLAY_QUIT: return "PLAY_QUIT";
        case PR_EDIT_TOGGLE_MODE: return "EDIT_TOGGLE_MODE";
        case PR_EDIT_SAVE_MAP: return "EDIT_SAVE_MAP";
        case PR_EDIT_OBJ_DELETE: return "EDIT_OBJ_DELETE";
        case PR_EDIT_OBJ_CREATE: return "EDIT_OBJ_CREATE";
        case PR_EDIT_OBJ_DUPLICATE: return "EDIT_OBJ_DUPLICATE";
        case PR_EDIT_PLANE_RESET: return "EDIT_PLANE_RESET";
        case PR_EDIT_MOVE_UP: return "EDIT_MOVE_UP";
        case PR_EDIT_MOVE_DOWN: return "EDIT_MOVE_DOWN";
        case PR_EDIT_MOVE_LEFT: return "EDIT_MOVE_LEFT";
        case PR_EDIT_MOVE_RIGHT: return "EDIT_MOVE_RIGHT";
        case PR_EDIT_OBJ_PROPERTIES_SHOW_TOGGLE: return "EDIT_OBJ_PROPERTIES_SHOW_TOGGLE";
        case PR_EDIT_OBJ_PROPERTY_LEFT: return "EDIT_OBJ_PROPERTY_LEFT";
        case PR_EDIT_OBJ_PROPERTY_RIGHT: return "EDIT_OBJ_PROPERTY_RIGHT";
        case PR_EDIT_OBJ_PROPERTY_TOGGLE: return "EDIT_OBJ_PROPERTY_TOGGLE";
        case PR_EDIT_OBJ_PROPERTY_NEXT_INCREMENT: return "EDIT_OBJ_PROPERTY_NEXT_INCREMENT";
        case PR_EDIT_OBJ_PROPERTY_INCREASE: return "EDIT_OBJ_PROPERTY_INCREASE";
        case PR_EDIT_OBJ_PROPERTY_DECREASE: return "EDIT_OBJ_PROPERTY_DECREASE";
        case PR_EDIT_OBJ_SELECTION_UP: return "EDIT_OBJ_SELECTION_UP";
        case PR_EDIT_OBJ_SELECTION_DOWN: return "EDIT_OBJ_SELECTION_DOWN";
        case PR_EDIT_OBJ_SELECTION_LEFT: return "EDIT_OBJ_SELECTION_LEFT";
        case PR_EDIT_OBJ_SELECTION_RIGHT: return "EDIT_OBJ_SELECTION_RIGHT";
        case PR_EDIT_OBJ_DESELECT: return "EDIT_OBJ_DESELECT";
        default: return "UNKNOWN";
    }
}

// Save and load keybindings from memory
int keybindings_save_to_file(const char *file_path,
        InputAction *actions,
        int actions_len) {
    int result = 0;
    FILE *key_file = NULL;

    {
        key_file = std::fopen(file_path, "wb");
        if (key_file == NULL) return_defer(1);

        std::fprintf(key_file, "%i\n", actions_len);
        if (std::ferror(key_file)) return_defer(2);

        for(int bind_index = 0; bind_index < actions_len; ++bind_index) {
            InputAction action = actions[bind_index];

            std::fprintf(key_file,
                         "%i:%i,%i:%i_%i,%i_%i\n",
                         bind_index,
                         action.kb_binds[0].bind_index,
                         action.kb_binds[1].bind_index,
                         (int)action.gp_binds[0].type,
                         action.gp_binds[0].bind_index,
                         (int)action.gp_binds[1].type,
                         action.gp_binds[1].bind_index);
            if (std::ferror(key_file)) return_defer(3);
        }
    }

    defer:
    if (key_file) std::fclose(key_file);
    return result;
}

int keybindings_load_from_file(const char *file_path,
        InputAction *actions,
        int actions_len) {
    int result = 0;
    int ret = 0;
    FILE *key_file = NULL;

    {
        key_file = std::fopen(file_path, "rb");
        if (key_file == NULL) return_defer(1);

        int key_file_bindings_num = 0;
        ret = std::fscanf(key_file, "%i", &key_file_bindings_num);
        if (ret != 1) return_defer(2);
        if (std::ferror(key_file)) return_defer(3);

        int upper_limit =
            (key_file_bindings_num < actions_len) ?
                key_file_bindings_num : actions_len;

        for(int i = 0; i < upper_limit; ++i) {

            int bind_index = -1;
            ret = std::fscanf(key_file, " %d:", &bind_index);
            if (ret != 1) return_defer(4);
            if (std::ferror(key_file)) return_defer(5);

            if (bind_index < 0 || bind_index >= actions_len) return_defer(6);

            InputAction *action = &actions[bind_index];

            ret = std::fscanf(key_file,
                              "%d,%d:%d_%d,%d_%d",
                              &action->kb_binds[0].bind_index,
                              &action->kb_binds[1].bind_index,
                              (int *)&action->gp_binds[0].type,
                              &action->gp_binds[0].bind_index,
                              (int *)&action->gp_binds[1].type,
                              &action->gp_binds[1].bind_index);
            if (ret != 6) return_defer(7);
            if (std::ferror(key_file)) return_defer(8);
        }
    }

    defer:
    if (key_file) std::fclose(key_file);
    return result;
}
