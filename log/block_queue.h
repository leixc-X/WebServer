#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

/* 阻塞队列：多个线程访问一个线程如果为空，那么获取队列元素的方法就会被阻塞，知道队列中存在元素可以获取
 * 阻塞队列主要解决线程安全问题，以下实现基于生产者 -消费者模型
 * push方法为生产者，pop方法是消费者
 * 当队列为空时，从队列中获取元素的线程将会被挂起；当队列是满时，往队列里添加元素的线程将会挂起。
 * 保证线程安全，每个操作方法都要先加互斥锁，操作完成后在释放
 */

#include "../lock/locker.h"
#include <cstddef>
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
using namespace std;

template <class T> class block_queue {

public:
  block_queue(int max_size = 1000) {
    if (max_size <= 0) {
      exit(-1);
    }

    m_max_size = max_size;
    m_array = new T[max_size];
    m_size = 0;
    m_front = -1;
    m_back = -1;
  }

  ~block_queue() {
    m_mutex.lock();
    if (m_array != NULL) {
      delete[] m_array;
    }
    m_mutex.unlock();
  }

  void clear() {
    m_mutex.lock();
    m_size = 0;
    m_front = -1;
    m_back = -1;
    m_mutex.unlock();
  }

  bool isFull() {
    m_mutex.lock();
    if (m_size >= m_max_size) {
      m_mutex.lock();
      return true;
    }
    m_mutex.unlock();
    return false;
  }

  bool isEmpty() {
    m_mutex.lock();
    if (0 == m_size) {
      m_mutex.lock();
      return true;
    }
    m_mutex.unlock();
    return false;
  }

  //返回队首元素
  bool front(T &value) {
    m_mutex.lock();
    if (0 == m_size) {
      m_mutex.unlock();
      return false;
    }
    value = m_array[m_front];
    m_mutex.unlock();
    return true;
  }

  //返回队尾元素
  bool back(T &value) {
    m_mutex.lock();
    if (0 == m_size) {
      m_mutex.unlock();
      return false;
    }
    value = m_array[m_back];
    m_mutex.unlock();
    return true;
  }

  int size() {
    int tmp = 0;
    m_mutex.lock();
    tmp = m_size;

    m_mutex.unlock();
    return tmp;
  }

  int max_size() {
    int tmp = 0;

    m_mutex.lock();
    tmp = m_max_size;

    m_mutex.unlock();
    return tmp;
  }

  /* 队列中添加元素，会使所有使用队列线程被唤醒
   * 当有元素push到队列时，相当于生产者生产了一个元素
   * 若当前没有等待条件变量，则唤醒无意义
   */
  bool push(const T &item) {
    m_mutex.lock();

    if (m_size >= m_max_size) {
      m_cond.broadcast();
      m_mutex.unlock();
      return false;
    }

    m_back = (m_back + 1) % m_max_size;
    m_array[m_back] = item;

    m_size++;

    m_cond.broadcast();
    m_mutex.unlock();
    return true;
  }

  /* 如队列pop时，没有元素，将会等待条件变量 */
  bool pop(T &item) {
    m_mutex.lock();
    while (m_size <= 0) {
      if (!m_cond.wait(m_mutex.get())) {
        m_mutex.unlock();
        return false;
      }
    }

    m_front = (m_front + 1) % m_max_size;
    item = m_array[m_front];
    m_size--;

    m_mutex.unlock();
    return true;
  }

  //增加了超时处理
  bool pop(T &item, int ms_timeout) {
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    m_mutex.lock();
    if (m_size <= 0) {
      t.tv_sec = now.tv_sec + ms_timeout / 1000;
      t.tv_nsec = (ms_timeout % 1000) * 1000;
      if (!m_cond.timewait(m_mutex.get(), t)) {
        m_mutex.unlock();
        return false;
      }
    }

    if (m_size <= 0) {
      m_mutex.unlock();
      return false;
    }

    m_front = (m_front + 1) % m_max_size;
    item = m_array[m_front];
    m_size--;
    m_mutex.unlock();
    return true;
  }

private:
  locker m_mutex;
  cond m_cond;

  T *m_array; // 可变数组实现队列
  int m_size;
  int m_max_size;
  int m_front;
  int m_back;
};

#endif