#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

/* 使用线程同步机制包装类 */
#include "../lock/locker.h"

/* 线程池类 */
template <typename T> class threadpool {

public:
  /* 参数thread_number
   * 表示线程中线程数量，max_requests是请求队列中最多允许的、等待处理请求数量 */
  threadpool(int thread_number = 8, int max_requests = 10000);
  ~threadpool();

  /* 往请求队列中添加任务 */
  bool append(T *request);

private:
  /* 工作线程运行的函数，它不断从工作队列中取出任务执行 */
  static void *worker(void *arg);
  void run();

private:
  int m_thread_number;  /* 线程池中的线程数 */
  int m_max_requests;   /* 请求队列中允许的最大请求数 */
  pthread_t *m_threads; /* 描述线程池数组，大小为m_thread_number */
  std::list<T *> m_workqueue; /* 请求队列 */
  locker m_queuelocker;       /* 保护请求队列的互斥锁 */
  sem m_queuestat;            /* 是否有任务需要处理 */
  bool m_stop;                /* 结束进程 */
};

template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests)
    : m_thread_number(thread_number), m_max_requests(max_requests),
      m_stop(false), m_threads(NULL) {
  if ((thread_number <= 0) || (max_requests <= 0)) {
    throw std::exception();
  }

  m_threads = new pthread_t[m_thread_number];
  if (!m_threads) {
    throw std::exception();
  }
  /* 创建 thread_number 个线程，并设置为脱离线程 */
  for (int i = 0; i < thread_number; ++i) {
    printf("create the %dth thread\n", i);
    if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
      delete[] m_threads;
      throw std::exception();
    }
    if (pthread_detach(m_threads[i])) {
      delete[] m_threads;
      throw std::exception();
    }
  }
}

template <typename T> threadpool<T>::~threadpool() {
  delete[] m_threads;
  m_stop = true;
}

template <typename T> bool threadpool<T>::append(T *request) {
  /* 操作工作队列时一定要加锁，因为它被所有线程共享 */
  m_queuelocker.lock();
  if (m_workqueue.size() > m_max_requests) {
    m_queuelocker.unlock();
    return false;
  }
  m_workqueue.push_back(request);
  m_queuelocker.unlock();
  m_queuestat.post();
  return true;
}

template <typename T> void *threadpool<T>::worker(void *arg) {
  threadpool *pool = (threadpool *)arg;
  pool->run();
  return pool;
}

template <typename T> void threadpool<T>::run() {
  while (!m_stop) {
    m_queuestat.wait();
    m_queuelocker.lock();
    if (m_workqueue.empty()) {
      m_queuelocker.unlock();
      continue;
    }
    T *request = m_workqueue.front();
    m_workqueue.pop_front();
    m_queuelocker.unlock();
    if (!request) {
      continue;
    }
    request->process();
  }
}

#endif