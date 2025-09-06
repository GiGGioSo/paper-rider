#ifndef PP_SHADERER_H
#define PP_SHADERER_H

#include "glm/glm.hpp"
#include "pr_mathy.h"

// NOTE: Might need to be a struct if something else might be needed
typedef unsigned int PR_Shader;

void shaderer_create_program(PR_Shader* s, const char* vertexPath, const char* fragmentPath);

void shaderer_set_int(PR_Shader s, const char* name, int value);
void shaderer_set_float(PR_Shader s, const char* name, float value);
void shaderer_set_mat4(PR_Shader s, const char* name, glm::mat4 value);
void shaderer_set_vec3(PR_Shader s, const char* name, vec3f value);

#endif
