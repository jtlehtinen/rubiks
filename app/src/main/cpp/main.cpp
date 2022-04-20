#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android/log.h>
#include <jni.h>
#include "android_native_app_glue.h"
#include <math.h>
#include <time.h>
#include <vector>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "rubiks", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "rubiks", __VA_ARGS__))

struct Color {
  float r, g, b, a;
};

struct SavedState {
  Color color;
};

struct Application {
  android_app *app;
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  int32_t width;
  int32_t height;
  SavedState state;
  bool animating;
};

// init_display initializes OpenGL ES and EGL context.
static int init_display(Application* application) {

  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(display, nullptr, nullptr);

  const EGLint attribs[] = {
    EGL_SURFACE_TYPE,
    EGL_WINDOW_BIT,
    EGL_BLUE_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_RED_SIZE,
    8,
    EGL_NONE,
  };

  EGLint config_count;
  eglChooseConfig(display, attribs, nullptr, 0, &config_count);

  std::vector<EGLConfig> configs(config_count);
  eglChooseConfig(display, attribs, configs.data(), configs.size(), &config_count);

  if (config_count == 0) {
    LOGW("Unable to initialize EGLConfig");
    return -1;
  }

  EGLConfig config = configs.front();

  for (auto& c : configs) {
    EGLint r, g, b, d;

    if (eglGetConfigAttrib(display, c, EGL_RED_SIZE, &r) &&
        eglGetConfigAttrib(display, c, EGL_GREEN_SIZE, &g) &&
        eglGetConfigAttrib(display, c, EGL_BLUE_SIZE, &b) &&
        eglGetConfigAttrib(display, c, EGL_DEPTH_SIZE, &d) &&
        r == 8 && g == 8 && b == 8 && d == 0) {
      config = c;
      break;
    }
  }

  EGLint format;
  eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

  EGLSurface surface = eglCreateWindowSurface(display, config, application->app->window, nullptr);
  EGLContext context = eglCreateContext(display, config, nullptr, nullptr);

  if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
    LOGW("Unable to eglMakeCurrent");
    return -1;
  }

  EGLint w, h;
  eglQuerySurface(display, surface, EGL_WIDTH, &w);
  eglQuerySurface(display, surface, EGL_HEIGHT, &h);

  application->display = display;
  application->context = context;
  application->surface = surface;
  application->width = w;
  application->height = h;
  application->state.color = {0.0f, 0.0f, 0.0f, 1.0f};

  for (auto name : {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS}) {
    auto info = glGetString(name);
    LOGI("OpenGL Info: %s", info);
  }

  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glEnable(GL_CULL_FACE);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_DEPTH_TEST);

  return 0;
}

static void draw_frame(Application* application) {
  if (application->display == nullptr) {
    return;
  }

  auto color = application->state.color;
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT);
  eglSwapBuffers(application->display, application->surface);
}

static void term_display(Application *application) {
  if (application->display != EGL_NO_DISPLAY) {
    eglMakeCurrent(application->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (application->context != EGL_NO_CONTEXT) {
      eglDestroyContext(application->display, application->context);
    }
    if (application->surface != EGL_NO_SURFACE) {
      eglDestroySurface(application->display, application->surface);
    }
    eglTerminate(application->display);
  }
  application->animating = false;
  application->display = EGL_NO_DISPLAY;
  application->context = EGL_NO_CONTEXT;
  application->surface = EGL_NO_SURFACE;
}

static int32_t handle_input(android_app *app, AInputEvent *event) {
  auto *application = (Application *)app->userData;
  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    application->animating = true;
    return 1;
  }
  return 0;
}

static void handle_command(android_app *app, int32_t cmd) {
  auto *application = (Application *)app->userData;
  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      application->app->savedState = malloc(sizeof(SavedState));
      *((SavedState *)application->app->savedState) = application->state;
      application->app->savedStateSize = sizeof(SavedState);
      break;

    case APP_CMD_INIT_WINDOW:
      if (application->app->window != nullptr) {
        init_display(application);
        draw_frame(application);
      }
      break;

    case APP_CMD_TERM_WINDOW:
      term_display(application);
      break;

    case APP_CMD_GAINED_FOCUS:
      break;

    case APP_CMD_LOST_FOCUS:
      application->animating = false;
      draw_frame(application);
      break;

    default:
      break;
  }
}

void android_main(android_app *state) {
  Application application = { };
  state->userData = &application;
  state->onAppCmd = handle_command;
  state->onInputEvent = handle_input;
  application.app = state;
  application.animating = true;

  const bool restore = state->savedState != nullptr;
  if (restore) {
    application.state = *(SavedState *)state->savedState;
  }

  while (true) {
    int events;
    android_poll_source *source;

    int timeout = application.animating ? 0 : -1;
    while (ALooper_pollAll(timeout, nullptr, &events, (void **)&source) >= 0) {
      if (source != nullptr) {
        source->process(state, source);
      }

      if (state->destroyRequested != 0) {
        term_display(&application);
        return;
      }
    }

    if (application.animating) {
      application.state.color.r += 0.01f;
      if (application.state.color.r >= 1.0f) {
        application.state.color.r = 0.0f;
      }

      // Drawing is throttled to the screen update rate.
      draw_frame(&application);
    }
  }
}
