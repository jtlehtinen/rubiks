#pragma once

#include <stdint.h>
#include <EGL/egl.h>
#if __ANDROID_API__ >= 24
  #include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
  #include <GLES3/gl31.h>
#else
  #include <GLES3/gl3.h>
#endif

struct Color {
  float r, g, b, a;
};

using Shader = uint32_t;
using ShaderProgram = uint32_t;

constexpr Shader kInvalidShader = ~(0u);
constexpr ShaderProgram kInvalidShaderProgram = ~(0u);

struct Renderer {
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  int32_t width;
  int32_t height;

  bool init(ANativeWindow *window);
  void deinit();
  bool can_draw();
  void swap_buffers();

  void clear(float r, float g, float b, float a);

  ShaderProgram create_shader_program(const char* vertex_shader_source, const char* fragment_shader_source);
};
