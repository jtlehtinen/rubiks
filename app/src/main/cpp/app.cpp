#include "app.h"
#include "log.h"

static const char* vertex_shader_source =
R"(#version 300 es

void main() {
  float x = -1.0 + float((gl_VertexID & 1) << 2);
  float y = -1.0 + float((gl_VertexID & 2) << 1);
  gl_Position = vec4(x, y, 0, 1.0);
}
)";

static const char* fragment_shader_source =
R"(#version 300 es

precision highp float;

uniform vec2 u_resolution;

out vec4 o_color;

vec4 sphere1 = vec4(0.0, 1.0, 0.0, 1.0);

vec3 sphere_normal(vec3 pos, vec4 sphere) {
  return (pos - sphere.xyz) / sphere.w;
}

float intersect_sphere(vec3 ro, vec3 rd, vec4 sphere) {
  vec3 oc = ro - sphere.xyz;
  float b = 2.0 * dot(oc, rd);
  float c = dot(oc, oc) - sphere.w * sphere.w;
  float h = b * b - 4.0 * c;
  if (h < 0.0) return -1.0;
  float t = (-b - sqrt(h)) / 2.0;
  return t;
}

float intersect(vec3 ro, vec3 rd) {
  return intersect_sphere(ro, rd, sphere1);
}

float half_lambert(vec3 N, vec3 L) {
  return pow(dot(N, L) * 0.5 + 0.5, 2.0);
}

float lambert(vec3 N, vec3 L) {
  return clamp(dot(N, L), 0.0, 1.0);
}

void main() {
  vec2 fragCoord = gl_FragCoord.xy;

  vec2 uv = fragCoord.xy / u_resolution.xy * 2.0 - 1.0;
  uv.x *= u_resolution.x / u_resolution.y;

  vec3 ro = vec3(0.0, 1.0, 3.0);
  vec3 rd = normalize(vec3(uv, -1.0));

  float t = intersect(ro, rd);

  vec3 col = vec3(0.1);
  if (t > 0.0) {
    vec3 P = ro + t * rd;
    vec3 N = sphere_normal(P, sphere1);
    vec3 L = normalize(vec3(0.57703));

  #if 1
    col = half_lambert(N, L) * vec3(1.0);
  #else
    col = lambert(N, L) * vec3(1.0);
  #endif
  }

	o_color = vec4(sqrt(col), 1.0);
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
  glUniform2f(glGetUniformLocation(program, "u_resolution"), renderer.width, renderer.height);

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
