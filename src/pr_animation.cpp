#include "pr_animation.h"

void animation_init(PR::Animation *a, Texture tex,
                    size_t start_x, size_t start_y,
                    size_t dim_x, size_t dim_y,
                    size_t step_x, size_t step_y,
                    size_t frame_number, float frame_duration, bool loop) {
    a->loop = loop;
    a->frame_number = frame_number;
    a->frame_stop = frame_number - 1;
    a->frame_duration = frame_duration;

    if (a->tc) std::free(a->tc);
    a->tc = (TexCoords *) std::malloc(sizeof(TexCoords) * frame_number);

    for(size_t i = 0; i < frame_number; ++i) {
        a->tc[i] = texcoords_in_texture_space(start_x + (step_x * i),
                                              start_y + (step_y * i),
                                              dim_x, dim_y,
                                              tex, false);
    }

    a->current = 0;
    a->active = false;
    a->finished = false;
    a->frame_elapsed = 0.f;
}

void animation_step(PR::Animation *a) {
    float dt = glob->state.delta_time;

    if (!a->active) return;

    a->frame_elapsed += dt;
    if (a->frame_elapsed > a->frame_duration) {
        a->frame_elapsed -= a->frame_duration;
        if (a->loop) {
            a->current = (a->current + 1) % a->frame_number;
        } else {
            if (a->current < a->frame_number-1 &&
                 a->current < a->frame_stop) {
                a->current++;
            } else {
                a->finished = true;
                a->active = false;
            }
        }
    }
}

void animation_queue_render(Rect b, PR::Animation *a, bool inverse) {
    TexCoords tc = a->tc[a->current];
    if (inverse) {
        tc.ty += tc.th;
        tc.th = -tc.th;
    }
    renderer_add_queue_tex(b, tc, false);
}

void animation_reset(PR::Animation *a) {
    a->current = 0;
    a->active = false;
    a->finished = false;
    a->frame_elapsed = 0.f;
}
