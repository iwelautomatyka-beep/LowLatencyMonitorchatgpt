#pragma once
#include <android/log.h>
#ifndef LOG_TAG
#define LOG_TAG "AudioEngine"
#endif
#ifdef NDEBUG
  #define LOGI(...)
  #define LOGE(...)
#else
  #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
  #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif
