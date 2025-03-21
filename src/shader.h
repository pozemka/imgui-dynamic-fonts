#pragma once

#include <GL/glcorearb.h>

#include <string>


extern GLuint g_VertexShader;
extern GLuint g_FragmentShader;
extern GLuint g_ShaderProgram;

void shaderInit();
void shaderCleanup();

bool CheckShader(GLuint handle, const char *desc);

bool ApplyFragmentShader(const std::string& new_shader);

bool CheckProgram(GLuint handle, const char *desc);