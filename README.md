# WebServer

Linux下轻量级Web服务器，自学网络编程入门项目，后续会持续完善功能
* 使用线程池 + 非阻塞socket + epoll的并发模型
* 使用状态机解析HTTP请求报文，支持GET请求和POST请求
* 基于升序链表实现定时器，关闭超时的非活动连接
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态
* 使用cmake替换原来的makefile来构建项目。cmake代码注释可供学习使用


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
 * 可以使用cmake构建（可选）
   ```
   //根目录下执行
   rm -rf build/        # 删除项目原来的build文件
   cmake -B build       # 生成构建目录，-B 指定生成的构建系统代码放在 build 目录
   cmake --build build  # 执行构建
   ./server [ip] [prot] # 可执行文件默认生成目录是根目录，可在根目录下的CMakeLists中进行修改（详见注释）
   ```

### 代码结构
```
├── build 
│   ├── cmake构建目录
├── CGImysql
│   ├── CMakeLists.txt
│   ├── sql_connection_pool.cpp
│   └── sql_connection_pool.h
├── CMakeLists.txt
├── http
│   ├── CMakeLists.txt
│   ├── http_conn.cpp
│   └── http_conn.h
├── lib
│   ├── liblibHttp.a
│   ├── liblibLog.a
│   └── liblibSqlPool.a
├── lock
│   └── locker.h
├── log
│   ├── block_queue.h
│   ├── CMakeLists.txt
│   ├── log.cpp
│   └── log.h
├── main.cpp
├── makefile
├── README.md
├── root
│   ├── xxx资源
├── server
├── test_presure
├── threadpool
│   └── threadpool.h
└── timer
    └── lst_timer.h
```

### 参考

Linux高性能服务器编程，游双著.

[TinyWebServer  qinguoyi]([链接地址](https://github.com/qinguoyi/TinyWebServer#%E6%A6%82%E8%BF%B0)) 
