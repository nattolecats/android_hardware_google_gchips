/*
 * Copyright (C) 2019 ARM Limited. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MALI_GRALLOC_LOG_H
#define MALI_GRALLOC_LOG_H

#ifndef LOG_TAG
#define LOG_TAG "mali_gralloc"
#endif

#include <mutex>
#include <string>
#include <vector>

#include <log/log.h>

enum class LogLevel {
  INFO,
  VERBOSE,
  WARNING,
  ERROR
};

void log_setup();
void log_later(LogLevel level, const char* fmt, ...);
void log_commit();
void log_info_verbose_as_warning();

#define MALI_GRALLOC_LOGI(...) log_later(LogLevel::INFO, __VA_ARGS__);
#define MALI_GRALLOC_LOGV(...) log_later(LogLevel::VERBOSE, __VA_ARGS__);
#define MALI_GRALLOC_LOGW(...) log_later(LogLevel::WARNING, __VA_ARGS__);
#define MALI_GRALLOC_LOGE(...) log_later(LogLevel::ERROR, __VA_ARGS__);

#endif
