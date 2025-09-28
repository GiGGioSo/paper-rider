#include "pr_shaderer.h"
#include "pr_common.h" // read_whole_file()

#include "glad/glad.h"

int32 shaderer_create_program(PR_Shader *s, const char* vertex_path, const char* fragment_path) {

    char *vshader_code = NULL;
    char *fshader_code = NULL;

    uint32 vertex = 0;
    uint32 fragment = 0;
    int32 result = 0;

    {
        vshader_code = read_whole_file(vertex_path);
        if (vshader_code == NULL) {
            fprintf(stderr,
                    "ERROR::SHADER::VERTEX::CODE_LOADING_FAILED (%s)\n",
                    vertex_path);
            return_defer(1);
        }

        fshader_code = read_whole_file(fragment_path);
        if (fshader_code == NULL) {
            fprintf(stderr,
                    "ERROR::SHADER::FRAGMENT::CODE_LOADING_FAILED (%s)\n",
                    vertex_path);
            return_defer(2);
        }


        // ----- compile shaders and create program -----
        int32 success;
        char log[512];

        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vshader_code, NULL);
        glCompileShader(vertex);
        // print compile errors if any
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(vertex, 512, NULL, log);
            fprintf(stderr,
                    "ERROR::SHADER::VERTEX::COMPILATION_FAILED (%s)\n"
                    "|------\n"
                    "| %s\n"
                    "|------\n",
                    vertex_path, log);
            return_defer(success);
        }

        // fragment shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fshader_code, NULL);
        glCompileShader(fragment);
        //print compile errors if any
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(fragment, 512, NULL, log);
            fprintf(stderr,
                    "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED (%s)\n"
                    "|------\n"
                    "| %s\n"
                    "|------\n",
                    fragment_path, log);
            return_defer(success);
        }

        // program
        *s = glCreateProgram();
        glAttachShader(*s, vertex);
        glAttachShader(*s, fragment);
        glLinkProgram(*s);
        // print linking errors if any
        glGetProgramiv(*s, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(*s, 512, NULL, log);
            fprintf(stderr,
                    "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                    "|------\n"
                    "| %s\n"
                    "|------\n",
                    log);
            return_defer(success);
        }

    }

    defer:
    if (vshader_code) {
        free(vshader_code);
        vshader_code = NULL;
    }
    if (fshader_code) {
        free(fshader_code);
        fshader_code = NULL;
    }
    if (vertex) {
        glDeleteShader(vertex);
        vertex = 0;
    }
    if (fragment) {
        glDeleteShader(fragment);
        fragment = 0;
    }
    return result;
}

void shaderer_set_int(PR_Shader s, const char* name, int value) {
    glUseProgram(s);
    glUniform1i(glGetUniformLocation(s, name), value);
}

void shaderer_set_float(PR_Shader s, const char* name, float value) {
    glUseProgram(s);
    glUniform1f(glGetUniformLocation(s, name), value);
}

void shaderer_set_vec3(PR_Shader s, const char* name, vec3f value) {
    glUseProgram(s);
    glUniform3f(glGetUniformLocation(s, name), value.x, value.y, value.z);
}

void shaderer_set_mat4(PR_Shader s, const char* name, mat4f value) {
    glUseProgram(s);
    glUniformMatrix4fv(glGetUniformLocation(s, name), 1, GL_TRUE, &value.e[0]);
}

