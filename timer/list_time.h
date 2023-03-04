#ifndef _LIST_TIME_H
#define _LIST_TIME_H

#include <netinet/in.h>
#include <stdio.h>
#include <time.h>

#define BUFFER_SIZE 64

class util_timer;

// 用户数据结构
struct client_data {
  sockaddr_in address;   // 客户端socket地址
  int sockfd;            // socket描述符
  char buf[BUFFER_SIZE]; // 读缓存
  util_timer *timer;     // 定时器
};

// 定时器类
class util_timer {

public:
  // 构造函数
  util_timer() : prev(nullptr), next(nullptr) {}

public:
  time_t expire; // 定时器超时时间 = 浏览器和服务器连接时刻 + 固定时间(TIMESLOT)
  void (*cb_func)(client_data *); // 回调函数
  client_data *user_data;         // 连接资源
  util_timer *prev;               // 前驱计时器
  util_timer *next;               // 后继计时器
};

// 定时器容器类,双向升序链表
class sort_timer_list {

public:
  sort_timer_list() : head(nullptr), tail(nullptr) {}
  // 链表被销毁时，需要删除每一个结点
  ~sort_timer_list() {
    util_timer *tmp = head;
    while (tmp) {
      head = tmp->next;
      delete tmp;
      tmp = head;
    }
  }

  void
  add_timer(util_timer
                *timer); // 将目标定时器添加到链表中，将定时器按超时时间升序插入
  void adjust_timer(
      util_timer *timer); // 任务发生变化的时候，调整定时器在链表中的位置
  void del_timer(util_timer *timer); // 删除定时器
  void tick(); // 定时任务处理函数，处理链表容器中到期的定时器

private:
  void add_timer(util_timer *timer, util_timer *lst_head); // 调整结点位置

  util_timer *head; // 头结点
  util_timer *tail; // 尾结点
};

void cb_func(
    client_data *
        user_data); // 定时器回调函数，它删除非活动连接socket上的注册事件，并关闭

#endif