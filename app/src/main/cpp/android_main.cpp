#include <jni.h>
#include "android_native_app_glue.h"
#include "app.h"
#include "log.h"
#include <math.h>
#include <time.h>
#include <vector>

struct Platform {
  android_app *android_app;
  App app;
  bool animating;
};

static int32_t handle_input(android_app *app, AInputEvent *event) {
  auto *platform = (Platform *)app->userData;
  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    platform->animating = true;
    return 1;
  }
  return 0;
}

static void handle_command(android_app *app, int32_t cmd) {
  auto *platform = (Platform *)app->userData;
  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      break;

    case APP_CMD_INIT_WINDOW:
      if (platform->android_app->window != nullptr) {
        platform->app.init_graphics(platform->android_app->window);
        platform->app.draw();
      }
      break;

    case APP_CMD_TERM_WINDOW:
      platform->app.deinit_graphics();
      platform->animating = false;
      break;

    case APP_CMD_GAINED_FOCUS:
      break;

    case APP_CMD_LOST_FOCUS:
      platform->animating = false;
      platform->app.draw();
      break;

    default:
      break;
  }
}

void android_main(android_app *state) {
  Platform platform = { };
  state->userData = &platform;
  state->onAppCmd = handle_command;
  state->onInputEvent = handle_input;
  platform.android_app = state;
  platform.animating = true;

  if (!platform.app.init()) {
    ANativeActivity_finish(state->activity);
    return;
  }

  while (true) {
    int events;
    android_poll_source *source;

    int timeout = platform.animating ? 0 : -1;
    while (ALooper_pollAll(timeout, nullptr, &events, (void **)&source) >= 0) {
      if (source != nullptr) {
        source->process(state, source);
      }

      if (state->destroyRequested != 0) {
        platform.app.deinit_graphics();
        platform.animating = false;
        return;
      }
    }

    platform.app.update();
    if (platform.animating) {
      platform.app.draw();
    }
  }

  platform.app.deinit();
}
