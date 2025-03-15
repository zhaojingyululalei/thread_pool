#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define LOG_QUEUE_SIZE 100  // 日志队列大小
#define LOG_MAX_BACKUP 5     // 旧日志文件最大备份

typedef struct {
    loglevel_t level;
    char message[1024];
} log_entry_t;

// 日志系统全局状态
static char log_filepath[256];
static loglevel_t log_level = LOGLEVEL_DEBUG;
static int log_max_size = 1024 * 1024;
static int log_fd = -1;

static log_entry_t log_queue[LOG_QUEUE_SIZE];
static int log_queue_head = 0, log_queue_tail = 0;

static lock_t log_lock;
static semaphore_t log_semaphore;
static tid_t log_thread;
static int log_running = 1;

// 获取当前时间字符串
static void get_current_time(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "[%H:%M:%S]", t);
}

// 获取日志级别字符串
static const char *get_level_str(loglevel_t level) {
    switch (level) {
        case LOGLEVEL_DEBUG:   return "[DEBUG]";
        case LOGLEVEL_INFO:    return "[INFO] ";
        case LOGLEVEL_WARNING: return "[WARN] ";
        case LOGLEVEL_ERROR:   return "[ERROR]";
        default:               return "[UNKNOWN]";
    }
}

// 滚动日志
static void rotate_log() {
    struct stat st;
    if (stat(log_filepath, &st) == 0 && st.st_size >= log_max_size) {
        for (int i = LOG_MAX_BACKUP; i > 0; i--) {
            char oldname[280], newname[280];
            snprintf(oldname, sizeof(oldname), "%s.%d", log_filepath, i - 1);
            snprintf(newname, sizeof(newname), "%s.%d", log_filepath, i);
            rename(oldname, newname);
        }

        close(log_fd);
        rename(log_filepath, log_filepath + 2);
        log_fd = open(log_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
}

// 日志写入线程
static void *log_writer_thread(void *arg) {
    while (log_running) {
        semaphore_wait(&log_semaphore);

        if (!log_running) break;

        lock(&log_lock);

        if (log_queue_head != log_queue_tail) {
            log_entry_t entry = log_queue[log_queue_tail];
            log_queue_tail = (log_queue_tail + 1) % LOG_QUEUE_SIZE;

            unlock(&log_lock);

            rotate_log();

            char time_buffer[10];
            get_current_time(time_buffer, sizeof(time_buffer));

            char log_buffer[1100];
            snprintf(log_buffer, sizeof(log_buffer), "%s %s %s\n", time_buffer, get_level_str(entry.level), entry.message);

            write(log_fd, log_buffer, strlen(log_buffer));
        } else {
            unlock(&log_lock);
        }
    }

    return NULL;
}

// 初始化日志系统
void log_init(const char *log_file,  int max_size) {
    strncpy(log_filepath, log_file, sizeof(log_filepath) - 1);
    log_max_size = max_size;

    log_fd = open(log_filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    lock_init(&log_lock);
    semaphore_init(&log_semaphore, 0);
    log_running = 1;
    log_thread = thread_create(log_writer_thread, NULL);
}

// 记录日志（异步）
void log_message(loglevel_t level, const char *format, ...) {
    if (level < log_level) return;

    char message[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    lock(&log_lock);

    int next_pos = (log_queue_head + 1) % LOG_QUEUE_SIZE;
    if (next_pos != log_queue_tail) {
        log_queue[log_queue_head].level = level;
        strncpy(log_queue[log_queue_head].message, message, sizeof(log_queue[log_queue_head].message) - 1);
        log_queue_head = next_pos;
        semaphore_post(&log_semaphore);
    }

    unlock(&log_lock);
}

// 关闭日志系统
void log_close() {
    log_running = 0;
    semaphore_post(&log_semaphore);
    thread_join(log_thread);
    close(log_fd);
    lock_destroy(&log_lock);
    semaphore_destroy(&log_semaphore);
}
