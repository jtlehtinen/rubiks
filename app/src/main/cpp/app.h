#pragma once

#include "renderer.h"

struct App {
  Renderer renderer;
  ShaderProgram program = kInvalidShaderProgram;
  GLuint vao = 0;

  bool init();
  void deinit();

  bool init_graphics(ANativeWindow *window);
  void deinit_graphics();

  void update();
  void draw();
};
