#include "pr_parallax.h"

void parallax_init(PR_Parallax *px, float fc, PR_TexCoords tc,
                   float p_start_x, float p_start_y,
                   float p_w, float p_h) {

    px->tex_coords = tc;
    px->follow_coeff = fc;
    // NOTE: The reference is the middle of the middle piece at the start
    px->reference_point = p_start_x + p_w * 1.5f;

    for(size_t piece_index = 0;
        piece_index < ARRAY_LENGTH(px->pieces);
        ++piece_index) {

        PR_ParallaxPiece *piece = px->pieces + piece_index;

        *piece = {
            .base_pos_x = p_start_x + (p_w * piece_index),
            .body = {
                .pos = glm::vec2(p_start_x + (p_w * piece_index), p_start_y),
                .dim = glm::vec2(p_w, p_h),
                .angle = 0.f,
                .triangle = false,
            }
        };
    }
}

void parallax_update_n_queue_render(PR_Parallax *px, float current_x) {
    for(size_t piece_index = 0;
        piece_index < ARRAY_LENGTH(px->pieces);
        ++piece_index) {

        PR_ParallaxPiece *piece = px->pieces + piece_index;
        piece->body.pos.x = piece->base_pos_x +
                            (px->reference_point - current_x) *
                            (1.f - px->follow_coeff);

        // NOTE: px->reference_point - piece->body.dim.x * 1.5f
        //          would be the starting pos.x of the first piece.
        //       px->reference_point - piece->body.dim.x * 1.5f
        //          means half the width to the left of the first piece
        //       In that case we change that piece so that piece
        //          becomes the last one
        if (piece->body.pos.x <
                px->reference_point - piece->body.dim.x * 2.f) {
            piece->base_pos_x += piece->body.dim.x * 3.f;
        }
        if (piece->body.pos.x + piece->body.dim.x >
                px->reference_point + piece->body.dim.x * 2.f) {
            piece->base_pos_x -= piece->body.dim.x * 3.f;
        }
        renderer_add_queue_tex(piece->body, px->tex_coords, false);
    }
}
