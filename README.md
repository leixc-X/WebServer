# WebServer

Linux下轻量级Web服务器，自学网络编程入门项目，后续会持续完善功能
* 使用线程池 + 非阻塞socket + epoll的并发模型
* 使用状态机解析HTTP请求报文，支持GET请求和POST请求
* 基于升序链表实现定时器，关闭超时的非活动连接
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态


### 环境要求

* 服务器测试环境
   * Ubuntu 20.04
   * C/C++
  
* 浏览器测试环境
	* Windows、Linux
	* Chrome、FireFox

### 快速运行

* 修改资源路径地址      ./http/http_conn.cpp
  
  ``` 
  /* 网站根目录 */  const char *doc_root = "/home/xxx/linuxWebServer/root";
  ```
  
 * 项目根目录
 
    ``` 
    make server
    ```
 * 启动server
 
    ``` 
    ./server [ip] [port]
    ```
 * 浏览器端
 
   ```
   ip:port/[目标资源]
   ```


### 代码结构

    .
    ├── http            http连接处理类
    │   ├── http_conn.cpp
    │   └── http_conn.h
    ├── timer           定时升序链表类
    │   ├── list_time.cpp
    │   └── list_time.h
    ├── log             日志
    │   ├── log.cpp
    │   └── log.h
    │   └── block_queue.h
    ├── lock            线程同步机制包装类
    │   └── locker.h
    ├── main.cpp        主入口
    ├── makefile
    ├── README.md
    ├── root            静态资源位置
    │   └── welcome.html
    ├── server          可执行文件
    └── threadpool      线程池
        ├── processpool.h
        └── threadpool.h


### 参考

Linux高性能服务器编程，游双著.

[TinyWebServer  qinguoyi]([链接地址](https://github.com/qinguoyi/TinyWebServer#%E6%A6%82%E8%BF%B0)) 
