#include "pr_ui.h"
#include "pr_rect.h"

bool button_clicked(PR_InputController *input, PR_Button button) {
    if (!input) return false;

    return (input->mouse_left.clicked &&
            rect_contains_point(button.body,
                                input->mouseX, input->mouseY,
                                button.from_center));
}
