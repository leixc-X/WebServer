#include "list_time.h"


/*将目标定时器timer添加到链表中*/
void sort_timer_list::add_timer(util_timer *timer) {
  if (!timer) {
    return;
  }

  // 如果链表是空的则添加结点
  if (!head) {
    head = tail = timer;
    return;
  }

  /* 如果目标定时器的超时时间小于当前链表所有定时器，则把该结点加入链表头部，否则调用重载函数add_timer
   * 插入链表合适位置，保证升序 */
  if (timer->expire < head->expire) {
    timer->next = head;
    head->prev = timer;
    head = timer;
    return;
  }
  add_timer(timer, head);
}

void sort_timer_list::adjust_timer(util_timer *timer) {
  if (!timer) {
    return;
  }
  util_timer *tmp = timer->next;
  //  如果被调整的目标定时器处在链表尾部，或者定时器新的超时时间仍然小于下一个定时器的超时值，则不用调整
  if (!tmp || (timer->expire < tmp->expire)) {
    return;
  }
  // 如果目标定时器是链表头部，则该定时器从链表中取出并重新插入链表
  if (timer == head) {
    head = head->next;
    head->prev = nullptr;
    timer->next = nullptr;
    add_timer(timer, head);
  }

  // 如果不是头结点，则将该定时器取出然后插入原来所在位置之后的部分链表中
  else {
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    add_timer(timer, timer->next);
  }
}

void sort_timer_list::del_timer(util_timer *timer) {
  if (!timer) {
    return;
  }
  // 下面这个条件成立表示链表中只有一个定时器，及目标定时器
  if ((timer == head) && (timer == tail)) {
    delete timer;
    head = nullptr;
    tail = nullptr;
    return;
  }

  // 如果链表至少有两个定时器，且目标的定时器是头结点，则将链表头结点重置为原节点的下一个结点，然后删除目标定时器
  if (timer == head) {
    head = head->next;
    head->prev = nullptr;
    delete timer;
    return;
  }
  // 同样是两个，如果是尾，则将尾结点重置为原尾结点前一个结点，然后删除目标定时器
  if (timer == tail) {
    tail = tail->prev;
    tail->next = nullptr;
    delete timer;
    return;
  }
  // 中间位置，则需要串联前后
  timer->prev->next = timer->next;
  timer->next->prev = timer->prev;
  delete timer;
}

void sort_timer_list::tick() {
  if (!head) {
    return;
  }
  printf("timer tick \n");
  time_t cur = time(nullptr); // 获取当前系统时间
  util_timer *tmp = head;
  // 从头结点开始依次处理每个定时器，知道遇到一个尚未到期的定时器，这就是定时器核心逻辑
  while (tmp) {
    // 超时时间使用的是绝对时间，所以可以用系统当前时间作比较,判断是否到期
    if (cur < tmp->expire) {
      break;
    }

    // 调用回调函数，执行定时任务
    tmp->cb_func(tmp->user_data);
    // 执行完定时任务，将它从链表中删除，并重置链表头结点
    head = tmp->next;
    if (head) {
      head->prev = nullptr;
    }
    delete tmp;
    tmp = head;
  }
}

/* 重载版本 该函数表示将目标定时器timer添加到结点lst_head之后的部分链表中 */
void sort_timer_list::add_timer(util_timer *timer, util_timer *lst_head) {
  util_timer *prev = lst_head;
  util_timer *tmp = prev->next;

  /* 遍历lst_head
   * 结点之后的链表，直到找到一个超时时间大于目标定时器的超时时间结点，并将目标定时器插入该节点之前
   */
  while (tmp) {
    if (timer->expire < tmp->expire) {
      prev->next = timer;
      timer->next = tmp;
      tmp->prev = timer;
      timer->prev = prev;
      break;
    }
    prev = tmp;
    tmp = tmp->next;
  }
  /* 如果遍历完所有结点未找到，则插入表尾，并设置为链表新的尾结点 */
  if (!tmp) {
    prev->next = timer;
    timer->prev = prev;
    tmp->next = nullptr;
    tail = timer;
  }
}

