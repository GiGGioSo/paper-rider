#ifndef PR_GL_DEBUG_H
#define PR_GL_DEBUG_H

#include <stdio.h>

#include "glad/glad.h"

static inline const char *gl_error_string(GLenum error) {
    switch (error) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default: return "UNKNOWN_GL_ERROR";
    }
}

static inline void gl_check_error(const char *label,
                                  const char *file,
                                  int line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr,
                "[OPENGL ERROR] %s (%#x) at %s:%d: %s\n",
                gl_error_string(error), error, file, line, label);
    }
}

#define GL_CHECK(label) gl_check_error((label), __FILE__, __LINE__)

#endif // PR_GL_DEBUG_H
