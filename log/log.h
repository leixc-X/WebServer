#ifndef LOG_H
#define LOG_H

#include "block_queue.h"
#include <bits/types/FILE.h>
#include <iostream>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>

using namespace std;

class Log {

public:
  static Log *get_instance() {
    static Log instance;
    return &instance;
  }

  static void *flush_log_thread(void *args) {
    Log::get_instance()->async_write_log();
  }

  //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
  bool init(const char *file_name, int log_buf_size = 8192,
            int split_lines = 5000000, int max_queue_size = 0);

  void write_log(int level, const char *format, ...);

  void flush(void);

private:
  Log();
  virtual ~Log();

  void *async_write_log() {
    string single_log;

    /* 从阻塞队列中取出一个string日志，写入文件 */
    while (m_log_queue->pop(single_log)) {
      m_mutex.lock();
      fputs(single_log.c_str(), m_fp);
      m_mutex.unlock();
    }
  }

private:
  char dir_name[128]; /* log路径 */
  char log_name[128]; /* log文件名 */
  int m_split_lines;  /* log最大行数 */
  int m_log_buf_size; /* log缓冲区大小 */
  long long m_count;  /* log行数记录 */
  int m_today;        /* 记录日期 */
  FILE *m_fp;         /* log文件指针 */
  char *m_buf;

  block_queue<string> *m_log_queue; //阻塞队列
  bool m_is_async;                  //是否同步标志位
  locker m_mutex;
};

#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__)

#endif