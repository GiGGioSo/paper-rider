#include "pp_shaderer.h"

#include "../include/glad/glad.h"

#include <iostream>
#include <fstream>
#include <sstream>

void shaderer_create_program(Shader* s, const char* vertexPath, const char* fragmentPath) {
        // ----- get shaders from their path -----
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile; // vertex shader file
    std::ifstream fShaderFile; // fragment shader file

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        std::stringstream vShaderStream, fShaderStream;

        // read file contents into the string streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        // close files
        vShaderFile.close();
        fShaderFile.close();

        // convert the string stream into a string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

    } catch (std::ifstream::failure &e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ:\n" << e.what() << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();


    // ----- compile shaders and create program -----
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    //print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // program
    *s = glCreateProgram();
    glAttachShader(*s, vertex);
    glAttachShader(*s, fragment);
    glLinkProgram(*s);
    // print linking errors if any
    glGetProgramiv(*s, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(*s, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // delete the single shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void shaderer_set_int(Shader s, const char* name, int value) {
    glUseProgram(s);
    glUniform1i(glGetUniformLocation(s, name), value);
}

void shaderer_set_float(Shader s, const char* name, float value) {
    glUseProgram(s);
    glUniform1f(glGetUniformLocation(s, name), value);
}

void shaderer_set_vec3(Shader s, const char* name, glm::vec3 value) {
    glUseProgram(s);
    glUniform3f(glGetUniformLocation(s, name), value.x, value.y, value.z);
}

void shaderer_set_mat4(Shader s, const char* name, glm::mat4 value) {
    glUseProgram(s);
    glUniformMatrix4fv(glGetUniformLocation(s, name), 1, GL_FALSE, &value[0].x);
}

