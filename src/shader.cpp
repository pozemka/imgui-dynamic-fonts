#include "shader.h"

#include <GL/gl3w.h>
// #include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <imgui.h>
#include <imgui_internal.h>

GLuint g_VertexShader;
GLuint g_FragmentShader;
GLuint g_ShaderProgram;

void shaderInit()
{
    g_VertexShader                 = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vertexShaderCode = R"(
#version 330 core
precision mediump float; 
layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 UV; 
layout(location = 2) in vec4 Color;
//layout(location = 3) in ??? OurData;

uniform mat4 ProjMtx;
out vec2 Frag_UV; 
out vec4 Frag_Color;

void main() {
  Frag_UV = UV;
  Frag_Color = Color;
  gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
}
)";

const GLchar *fragmentShaderCode = R"(
#version 330

#define TIME_SPEED 0.5

uniform sampler2D tex;
uniform float time;

in vec2 Frag_UV;
in vec4 Frag_Color;


float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

float screenPxRange() {
	vec2 unitRange = vec2(6.0)/vec2(textureSize(tex, 0));
	vec2 screenTexSize = vec2(1.0)/fwidth(Frag_UV);
	return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

const vec4 fgColor = vec4(1.0);
const vec4 outlineColor = vec4(0.95, 0.4, 0.3, 1.0);

// Ranges:
// -0.3 < thickness < 0.3
// 0.0 < softness < 0.5
float thickness = -0.0;
float outlineThickness = 0.5;
float softness = 0.2;
float outlineSoftness = 0.5;

void main() {
	vec4 texel = texture(tex, Frag_UV);
	float a = texel.a;
	float dist = median(texel.r, texel.g, texel.b);
	if (dist <= 0.0001) {
		discard;
	}
	float pxRange = screenPxRange();
	dist -= 0.5;
	
	dist += thickness;

  	float bodyPxDist = pxRange * dist;
  	softness *= pxRange;
	float bodyOpacity = smoothstep(-0.5 - softness, 0.5 + softness, bodyPxDist);
	
	float charPxDist = pxRange * (dist + outlineThickness);
	outlineSoftness *= pxRange;
	float charOpacity = smoothstep(-0.5 - outlineSoftness, 0.5 + outlineSoftness, charPxDist);

	float outlineOpacity = charOpacity - bodyOpacity;
	
	vec3 color = mix(outlineColor.rgb, fgColor.rgb, bodyOpacity);
	float alpha = bodyOpacity * fgColor.a + outlineOpacity * outlineColor.a;
    alpha = 1.0;
	
	gl_FragColor = vec4(color, alpha);
    // gl_FragColor = vec4(abs(texel.rgb), alpha);
}
)";

    //Vertex Shader
    glShaderSource(g_VertexShader, 1, &vertexShaderCode, nullptr);
    glCompileShader(g_VertexShader);
    CheckShader(g_VertexShader, "GH-4174 vertex shader");

    //Fragment Shader
    ApplyFragmentShader(fragmentShaderCode);
}



void shaderCleanup()
{
    if (g_ShaderProgram && g_VertexShader) {
        glDetachShader(g_ShaderProgram, g_VertexShader);
    }
    if (g_ShaderProgram && g_FragmentShader) {
        glDetachShader(g_ShaderProgram, g_FragmentShader);
    }
    if (g_VertexShader) {
        glDeleteShader(g_VertexShader);
        g_VertexShader = 0;
    }
    if (g_FragmentShader) {
        glDeleteShader(g_FragmentShader);
        g_FragmentShader = 0;
    }
    if (g_ShaderProgram) {
        glDeleteProgram(g_ShaderProgram);
        g_ShaderProgram = 0;
    }
}

bool CheckShader(GLuint handle, const char *desc)
{
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: failed to compile %s!", desc);
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetShaderInfoLog(handle, log_length, NULL, (GLchar *)buf.begin());
        fprintf(stderr, "%s", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

bool CheckProgram(GLuint handle, const char *desc)
{
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: failed to link %s!", desc);
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetProgramInfoLog(handle, log_length, NULL, (GLchar *)buf.begin());
        fprintf(stderr, "%s", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

bool ApplyFragmentShader(const std::string &new_shader)
{
    bool res = true;
    //Fragment Shader
    g_FragmentShader                     = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *fragmentShaderCode     = static_cast<const GLchar *>(new_shader.data());
    const GLint   fragmentShaderCodeSize = new_shader.size();
    glShaderSource(g_FragmentShader, 1, &fragmentShaderCode, &fragmentShaderCodeSize);

    glCompileShader(g_FragmentShader);
    res |= CheckShader(g_FragmentShader, "GH-4174 fragment shader");

    g_ShaderProgram = glCreateProgram();
    glAttachShader(g_ShaderProgram, g_VertexShader);
    glAttachShader(g_ShaderProgram, g_FragmentShader);
    glLinkProgram(g_ShaderProgram);
    res |= CheckProgram(g_ShaderProgram, "GH-4174 shader program");
    return res;
}