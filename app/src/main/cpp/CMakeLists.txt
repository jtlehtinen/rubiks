cmake_minimum_required(VERSION 3.12)

set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
  "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(
  rubiks
  SHARED
  android_main.cpp
  android_native_app_glue.h
  android_native_app_glue.c
  app.cpp
  renderer.h
  renderer.cpp
)

target_link_libraries(
  rubiks
  android
  EGL
  GLESv3
  log
)

set_property(TARGET rubiks PROPERTY CXX_STANDARD 20)
set_property(TARGET rubiks PROPERTY C_STANDARD 11)
