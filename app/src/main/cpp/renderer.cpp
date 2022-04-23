#include "renderer.h"
#include <EGL/eglext.h>
#include "log.h"
#include <vector>

namespace {

  EGLConfig choose_config(EGLDisplay display) {
    const EGLint attribs[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
      EGL_BUFFER_SIZE, 32,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_RED_SIZE, 8,
      EGL_NONE,
    };

    EGLint count;
    eglChooseConfig(display, attribs, nullptr, 0, &count);

    std::vector<EGLConfig> configs(count);
    eglChooseConfig(display, attribs, configs.data(), configs.size(), &count);

    if (count == 0) {
      LOGW("Unable to initialize EGLConfig");
      return nullptr;
    }

    EGLConfig config = configs.front();

    for (auto& c : configs) {
      EGLint r, g, b, d;

      if (eglGetConfigAttrib(display, c, EGL_RED_SIZE, &r) &&
          eglGetConfigAttrib(display, c, EGL_GREEN_SIZE, &g) &&
          eglGetConfigAttrib(display, c, EGL_BLUE_SIZE, &b) &&
          eglGetConfigAttrib(display, c, EGL_DEPTH_SIZE, &d) &&
          r == 8 && g == 8 && b == 8 && d == 0)
      {
        config = c;
        break;
      }
    }

    return config;
  }

  void log_context_info() {
    for (auto name : {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS}) {
     auto info = glGetString(name);
      LOGI("OpenGL Info: %s", info);
    }
  }

  Shader create_shader(const char* source, GLenum shader_type) {
    auto s = glCreateShader(shader_type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);

    GLint status;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      char buf[2048] = {};
      glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
      LOGW("Failed to compile %s shader:\n%s\n",
        shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment", buf);

      glDeleteShader(s);
      s = kInvalidShader;
    }

    return s;
  }
}

bool Renderer::init(ANativeWindow *window) {
  display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(display, nullptr, nullptr);

  auto config = choose_config(display);
  if (!config) {
    return false;
  }

  EGLint format;
  eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

  surface = eglCreateWindowSurface(display, config, window, nullptr);

  EGLint attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION, 3,
    EGL_CONTEXT_MINOR_VERSION, 2,
    EGL_NONE
  };
  context = eglCreateContext(display, config, nullptr, attribs);

  if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
    LOGW("Unable to eglMakeCurrent");
    return false;
  }

  eglQuerySurface(display, surface, EGL_WIDTH, &width);
  eglQuerySurface(display, surface, EGL_HEIGHT, &height);

  log_context_info();

  return true;
}

void Renderer::deinit() {
  if (display != EGL_NO_DISPLAY) {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (context != EGL_NO_CONTEXT) {
      eglDestroyContext(display, context);
    }

    if (surface != EGL_NO_SURFACE) {
      eglDestroySurface(display, surface);
    }

    eglTerminate(display);
  }

  display = EGL_NO_DISPLAY;
  context = EGL_NO_CONTEXT;
  surface = EGL_NO_SURFACE;
}

bool Renderer::can_draw() {
  return display != nullptr;
}

void Renderer::swap_buffers() {
  eglSwapBuffers(display, surface);
}

void Renderer::clear(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

ShaderProgram Renderer::create_shader_program(const char* vertex_shader_source, const char* fragment_shader_source) {
  auto vs = create_shader(vertex_shader_source, GL_VERTEX_SHADER);
  if (vs == kInvalidShader) return kInvalidShaderProgram;

  auto fs = create_shader(fragment_shader_source, GL_FRAGMENT_SHADER);
  if (fs == kInvalidShader) {
    glDeleteShader(vs);
    return kInvalidShaderProgram;
  }

  auto p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);

  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint status;
  glGetProgramiv(p, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    char buf[2048] = {};
    glGetProgramInfoLog(p, sizeof(buf), nullptr, buf);
    LOGW("Shader program link failed:\n%s\n", buf);
    glDeleteProgram(p);
    p = kInvalidShaderProgram;
  }

  return p;
}
