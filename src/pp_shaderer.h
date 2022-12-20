#ifndef SHADERER_H
#define SHADERER_H

#include "../include/glm/glm.hpp"

// NOTE: Might need to be a struct if something else might be needed
typedef unsigned int Shader;

void shaderer_create_program(Shader* s, const char* vertexPath, const char* fragmentPath);

void shaderer_set_int(Shader s, const char* name, int value);
void shaderer_set_float(Shader s, const char* name, float value);
void shaderer_set_mat4(Shader s, const char* name, glm::mat4 value);
void shaderer_set_vec3(Shader s, const char* name, glm::vec3 value);

#endif
