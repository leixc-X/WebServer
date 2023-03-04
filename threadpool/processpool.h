#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* 描述一个子进程，m_pid 是目标子进程pid， m_pipefd表示父进程和子进程通信的管道
 */
class process {
public:
  process() : m_pid(-1) {}

public:
  pid_t m_pid;
  int m_pipefd[2];
};

/* 进程池类，模板增加复用性，模板参数表示处理逻辑任务的类 */
template <typename T> class processpool {
private:
  /* 构造函数为私有，后面只能通过create函数来调用创建 线程池实例 */
  processpool(int listenfd, int process_number = 8);

public:
  /* 单体模式，保证一个程序最多只有一个线程池实例对象，这是程序正确处理信号的必要条件
   */
  static processpool<T> *create(int listenfd, int process_number = 8) {
    if (!m_instance) {
      m_instance = new processpool<T>(listenfd, process_number);
    }
    return m_instance;
  }
  ~processpool() { delete[] m_sub_process; }

  /* 启动线程池 */
  void run();

private:
  void setup_sig_pipe();
  void run_parent();
  void run_child();

private:
  static const int MAX_PROCESS_NUMBER = 16; // 进程池允许的最大子进程数量
  static const int USER_PER_PROCESS = 65536; // 每个子进程最多处理客户端数量
  static const int MAX_EVENT_NUMBER = 10000; // epoll最多处理事件数
  int m_process_number;                      // 进程池中的进程总数
  int m_idx;              // 子进程在池中的序号，从0开始
  int m_epollfd;          // 每个进程都有一个epoll内核时间表
  int m_listenfd;         // 监听socket
  int m_stop;             // 子进程通过 m_stop 决定是否停止
  process *m_sub_process; // 保存所有进程描述信息
  static processpool<T> *m_instance; // 静态实例对象
};

template <typename T> processpool<T> *processpool<T>::m_instance = NULL;

/* 用于处理信号的管道，实现统一事件源，也叫信号管道 */
static int sig_pipefd[2];

/* 文件描述符设置非阻塞 */
static int setnonblocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

static void addfd(int epollfd, int fd) {
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLET;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  setnonblocking(fd);
}

/* 从epollfd标识的epoll内核事件表中删除fd上所有注册事件 */
static void removefd(int epollfd, int fd) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
  close(fd);
}

static void sig_handler(int sig) {
  int save_errno = errno;
  int msg = sig;
  send(sig_pipefd[1], (char *)&msg, 1, 0);
  errno = save_errno;
}

static void addsig(int sig, void(handler)(int), bool restart = true) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;
  if (restart) {
    sa.sa_flags |= SA_RESTART;
  }
  sigfillset(&sa.sa_mask);
  assert(sigaction(sig, &sa, NULL) != -1);
}

/* 进程池构造函数，参数listenfd
 * 监听socket，它必须在创建线程池之前被创建，否则进程无法直接引用，参数process_number
 * 值进程池中线程数量 */
template <typename T>
processpool<T>::processpool(int listenfd, int process_number)
    : m_listenfd(listenfd), m_process_number(process_number), m_idx(-1),
      m_stop(false) {
  assert((m_process_number > 0) && (m_process_number <= MAX_PROCESS_NUMBER));
  m_sub_process = new processpool(process_number);
  assert(m_sub_process);

  /* 创建process_number个子进程，并建立其与父进程之间的管道 */
  for (int i = 0; i < process_number; ++i) {
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
    assert(ret == 0);

    m_sub_process[i].m_pid = fork();
    assert(m_sub_process[i].m_pid >= 0);

    if (m_sub_process[i].m_pid > 0) {
      // 父进程
      close(m_sub_process[i].m_pipefd[1]); // 写
      continue;
    } else {
      // 子进程
      close(m_sub_process[i].m_pipefd[0]); // 读
      m_idx = i;
      break;
    }
  }
}

/* 统一事件源 */
template <typename T> void processpool<T>::setup_sig_pipe() {
  /* 创建epoll事件监听表和信号管道 */
  m_epollfd = epoll_create(5);
  assert(m_epollfd != -1);

  int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
  assert(ret != -1);

  setnonblocking(sig_pipefd[1]);
  addfd(m_epollfd, sig_pipefd[0]);

  /* 设置信号处理函数 */
  addsig(SIGHUP, sig_handler);
  addsig(SIGTERM, sig_handler);
  addsig(SIGINT, sig_handler);
  addsig(SIGPIPE, SIG_IGN);
}

/* 父进程m_idx 值为-1 ，子进程中m_idx 大于0
 * 根据这个来判断是运行父进程代码还是子进程代码 */
template <typename T> void processpool<T>::run() {
  if (m_idx != -1) {
    // 子进程
    run_child();
    return;
  }
  run_parent();
}

template <typename T> void processpool<T>::run_child() {
  setup_sig_pipe();

  /* 每个子进程都通过其在进程池中的序号m_idx 找到父进程通信的管道 */
  int pipefd = m_sub_process[m_idx].m_pipefd[1];

  /* 子进程需要监听管道文件描述符pipefd，
   * 因为父进程将通知它来通知子进程accept新连接 */
  addfd(m_epollfd, pipefd);

  epoll_event events[MAX_EVENT_NUMBER];
  T *users = new T[USER_PER_PROCESS];
  assert(users);
  int number = 0;
  int ret = -1;

  while (!m_stop) {
    number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
    if ((number < 0) && (errno != EINTR)) {
      printf("epoll failure\n");
      break;
    }

    for (int i = 0; i < number; i++) {
      int sockfd = events[i].data.fd;
      if ((sockfd == pipefd) && (events[i].events & EPOLLIN)) {
        int client = 0;
        /* 从父、子进程之间管道读取数据，并将结果保存client中，如果成功则表示有新的客户连接
         */
        ret = recv(sockfd, (char *)&client, sizeof(client), 0);
        if (((ret < 0) && (errno != EAGAIN)) || ret == 0) {
          continue;
        } else {
          struct sockaddr_in client_address;
          socklen_t client_addrlength = sizeof(client_address);
          int connfd = accept(m_listenfd, (struct sockaddr *)&client_address,
                              &client_addrlength);
          if (connfd < 0) {
            printf("errno is: %d\n", errno);
            continue;
          }
          addfd(m_epollfd, connfd);
          /* 模板类T必须实现init方法，初始化一个客户连接，直接使用connfd来索引逻辑处理对象，提高程序效率
           */
          users[connfd].init(m_epollfd, connfd, client_address);
        }

      } /* 处理子进程接收的信号 */
      else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
        int sig;
        char signals[1024];
        ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
        if (ret <= 0) {
          continue;
        } else {
          for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
            case SIGCHLD: {
              pid_t pid;
              int stat;
              while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                continue;
              }
              break;
            }
            case SIGTERM:
            case SIGINT: {
              m_stop = true;
              break;
            }
            default: {
              break;
            }
            }
          }
        }
      } /* 如果有其他可读数据，则必然是客户端请求的到来，调用逻辑处理对象的process方法处理之
         */
      else if (events[i].events & EPOLLIN) {
        users[sockfd].process();
      } else {
        continue;
      }
    }
  }

  delete[] users;
  users = NULL;
  close(pipefd);
  // close( m_listenfd );/*
  // 这里关闭 m_listenfd 的操作应该交给它的创建者，由谁创建就由谁销毁 */

  close(m_epollfd);
}

template <typename T> void processpool<T>::run_parent() {
  setup_sig_pipe();

  /* 父进程监听m_listenfd */
  addfd(m_epollfd, m_listenfd);

  epoll_event events[MAX_EVENT_NUMBER];
  int sub_process_counter = 0;
  int new_conn = 1;
  int number = 0;
  int ret = -1;

  while (!m_stop) {
    number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
    if ((number < 0) && (errno != EINTR)) {
      printf("epoll failure\n");
      break;
    }

    for (int i = 0; i < number; i++) {
      int sockfd = events[i].data.fd;
      if (sockfd == m_listenfd) {

        /* 如果有新的连接到来，就用轮流着选举方式分配一个子进程处理 */
        int i = sub_process_counter;
        do {
          if (m_sub_process[i].m_pid != -1) {
            break;
          }
          i = (i + 1) % m_process_number;
        } while (i != sub_process_counter);

        if (m_sub_process[i].m_pid == -1) {
          m_stop = true;
          break;
        }
        sub_process_counter = (i + 1) % m_process_number;

        send(m_sub_process[i].m_pipefd[0], (char *)&new_conn, sizeof(new_conn),
             0);
        printf("send request to child %d\n", i);

      } /* 处理父进程接收到的信号 */
      else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
        int sig;
        char signals[1024];
        ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
        if (ret <= 0) {
          continue;
        } else {
          for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
            case SIGCHLD: {
              pid_t pid;
              int stat;
              while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                for (int i = 0; i < m_process_number; ++i) {
                  /* 如果进程池中第i个子进程退出了，则主进程关闭相应的通信管道，并设置m_idx
                   * 为-1 标记退出的子进程 */
                  if (m_sub_process[i].m_pid == pid) {
                    printf("child %d join\n", i);
                    close(m_sub_process[i].m_pipefd[0]);
                    m_sub_process[i].m_pid = -1;
                  }
                }
              }
              /* 如果所有子进程退出则父进程也推出 */
              m_stop = true;
              for (int i = 0; i < m_process_number; ++i) {
                if (m_sub_process[i].m_pid != -1) {
                  m_stop = false;
                }
              }
              break;
            }
            case SIGTERM:
            case SIGINT: {
              /* 如果父进程接受到终止信号，那么杀死所有子进程，并等待他们全部结束。
               */
              printf("kill all the clild now\n");
              for (int i = 0; i < m_process_number; ++i) {
                int pid = m_sub_process[i].m_pid;
                if (pid != -1) {
                  kill(pid, SIGTERM);
                }
              }
              break;
            }
            default: {
              break;
            }
            }
          }
        }
      } else {
        continue;
      }
    }
  }

  // close( m_listenfd );
  close(m_epollfd);
}

#endif