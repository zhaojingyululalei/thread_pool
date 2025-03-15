#ifndef __LOG_H
#define __LOG_H

#include "plat.h"

// 日志级别
typedef enum {
    LOGLEVEL_DEBUG,
    LOGLEVEL_INFO,
    LOGLEVEL_WARNING,
    LOGLEVEL_ERROR
} loglevel_t;

// 初始化日志系统
void log_init(const char *log_file, int max_size);

// 记录日志
void log_message(loglevel_t level, const char *format, ...);

// 关闭日志系统
void log_close();

#endif