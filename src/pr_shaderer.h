#ifndef PP_SHADERER_H
#define PP_SHADERER_H

#include "pr_mathy.h"

// NOTE: Might need to be a struct if something else might be needed
typedef unsigned int PR_Shader;

int32 shaderer_create_program(PR_Shader* s, const char* vertex_path, const char* fragment_path);

void shaderer_set_int(PR_Shader s, const char* name, int value);
void shaderer_set_float(PR_Shader s, const char* name, float value);
void shaderer_set_mat4(PR_Shader s, const char* name, mat4f value);
void shaderer_set_vec3(PR_Shader s, const char* name, vec3f value);

#endif
