#include "app.h"
#include "log.h"

static const char* vertex_shader_source = R"(#version 300 es

out vec3 pass_color;

void main() {
  vec2[] p = vec2[](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(0.0, 0.5));
  vec3[] c = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

  pass_color = c[gl_VertexID];
  gl_Position = vec4(p[gl_VertexID], 0.0, 1.0);
}
)";

static const char* fragment_shader_source = R"(#version 300 es

precision highp float;

in vec3 pass_color;

out vec4 o_color;

void main() {
  o_color = vec4(pass_color, 1.0);
}
)";

bool App::init() {
  return true;
}

void App::deinit() {

}

void App::update() {

}

void App::draw() {
  if (!renderer.can_draw()) return;

  renderer.clear(0.2f, 0.5f, 0.7f, 1.0f);

  glUseProgram(program);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
  glUseProgram(0);

  renderer.swap_buffers();
}

bool App::init_graphics(ANativeWindow *window) {
  if (!renderer.init(window)) {
    return false;
  }

  program = renderer.create_shader_program(vertex_shader_source, fragment_shader_source);
  if (program == kInvalidShaderProgram) return false;

  if (glGetError() != GL_NO_ERROR) {
    LOGW("PERKELE");
  }

  glGenVertexArrays(1, &vao);

  return true;
}

void App::deinit_graphics() {
  glDeleteVertexArrays(1, &vao);
  vao = 0;

  glDeleteProgram(program);
  program = kInvalidShaderProgram;

  renderer.deinit();
}
