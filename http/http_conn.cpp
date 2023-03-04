#include "http_conn.h"

/* 定义HTTP相应状态信息 */
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form =
    "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form =
    "You do not have permission to get file from this server. \n";
const char *error_404_title = "Not Found";
const char *error_404_form =
    "The requested file was not found on this server. \n";
const char *error_500_title = "Internal Error";
const char *error_500_form =
    "There was an unusual problem serving the requested file. \n";

/* 网站根目录 */
const char *doc_root = "/home/lxc/coding/myProject/linuxWebServer/root";

int setnonblocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

void addfd(int epollfd, int fd, bool one_shot) {
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  if (one_shot) {
    event.events |= EPOLLONESHOT;
  }
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  setnonblocking(fd);
}

void removefd(int epollfd, int fd) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
  close(fd);
}

void modfd(int epollfd, int fd, int ev) {
  epoll_event event;
  event.data.fd = fd;
  event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
  epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::close_conn(bool real_close) {
  if (real_close && (m_sockfd != -1)) {
    // modfd( m_epollfd, m_sockfd, EPOLLIN );
    removefd(m_epollfd, m_sockfd);
    m_sockfd = -1;
    m_user_count--; /* 关闭一个连接时，客户数量减一 */
  }
}

void http_conn::init(int sockfd, const sockaddr_in &addr) {
  m_sockfd = sockfd;
  m_address = addr;
  /* 下面两行是为了避免TIME——WAIT状态，仅用于调试，实际使用需要去掉 */
  // int error = 0;
  // socklen_t len = sizeof( error );
  // getsockopt( m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len );
  // int reuse = 1;
  // setsockopt( m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
  addfd(m_epollfd, sockfd, true);
  m_user_count++;

  init();
}

void http_conn::init() {
  m_check_state = CHECK_STATE_REQUESTLINE;
  m_linger = false;

  m_method = GET;
  m_url = 0;
  m_version = 0;
  m_content_length = 0;
  m_host = 0;
  m_start_line = 0;
  m_checked_idx = 0;
  m_read_idx = 0;
  m_write_idx = 0;
  memset(m_read_buf, '\0', READ_BUFFER_SIZE);
  memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
  memset(m_real_file, '\0', FILENAME_LEN);
}

/* 从状态机 */

http_conn::LINE_STATUS http_conn::parse_line() {
  char temp;
  /* m_checked_idx
   * 指向buffer（读缓存）中正在分析的字节，m_read_idx指向buffer中客户数据尾部的下一个字节
   */
  for (; m_checked_idx < m_read_idx; ++m_checked_idx) {
    /* 获取当前要分析的字节 */
    temp = m_read_buf[m_checked_idx];
    /* 如果当前的字节是 \r （回车符）则说明可能是一个完整的行 */
    if (temp == '\r') {
      /* 如果 \r
       * 字符碰巧是目前buffer中最后一个已经被读入的客户数据，则表示本次没有获取一个完整的行，需要进一步分析
       */
      if ((m_checked_idx + 1) == m_read_idx) {
        return LINE_OPEN;
      } /* 如果下一个字符 \n 则表示读取到一个完整的行 */
      else if (m_read_buf[m_checked_idx + 1] == '\n') {
        m_read_buf[m_checked_idx++] = '\0';
        m_read_buf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      /* 否则表示请求语法有问题 */
      return LINE_BAD;
    } /* 如果当前字符是 \n 则也可能读取到一个完整的行 */
    else if (temp == '\n') {
      if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r')) {
        m_read_buf[m_checked_idx - 1] = '\0';
        m_read_buf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
  }
  /* 如果所有内容分析后没有遇到 \r ,则返回LINE_OPEN 继续获取客户端数据进一部分析
   */
  return LINE_OPEN;
}

/* 循环读取客户端数据，知道无数据可读或者对方关闭连接 */
bool http_conn::read() {
  if (m_read_idx >= READ_BUFFER_SIZE) {
    return false;
  }

  int bytes_read = 0;
  while (true) {
    bytes_read = recv(m_sockfd, m_read_buf + m_read_idx,
                      READ_BUFFER_SIZE - m_read_idx, 0);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      return false;
    } else if (bytes_read == 0) {
      return false;
    }

    m_read_idx += bytes_read;
  }
  return true;
}

/* 解析HTTP请求行，获取请求方法、目标URL、HTTP版本号 */
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
  m_url = strpbrk(text, " \t");
  /* 如果没有 \r 或者空白符，则HTTP请求必有问题 */
  if (!m_url) {
    return BAD_REQUEST;
  }
  *m_url++ = '\0';

  char *method = text;
  if (strcasecmp(method, "GET") == 0) {
    m_method = GET;
  } else {
    return BAD_REQUEST;
  }

  m_url += strspn(m_url, " \t");
  m_version = strpbrk(m_url, " \t");
  if (!m_version) {
    return BAD_REQUEST;
  }
  *m_version++ = '\0';
  m_version += strspn(m_version, " \t");
  if (strcasecmp(m_version, "HTTP/1.1") != 0) {
    return BAD_REQUEST;
  }

  if (strncasecmp(m_url, "http://", 7) == 0) {
    m_url += 7;
    m_url = strchr(m_url, '/');
  }

  if (!m_url || m_url[0] != '/') {
    return BAD_REQUEST;
  }
  /* HTTP请求行处理完毕，状态转移到头部字段的分析 */
  m_check_state = CHECK_STATE_HEADER;
  return NO_REQUEST;
}

/* 解析HTTP请求的一个头部信息 */
http_conn::HTTP_CODE http_conn::parse_headers(char *text) {
  /* 遇到空行，表示头部字段解析完毕 */
  if (text[0] == '\0') {
    if (m_method == HEAD) {
      return GET_REQUEST;
    }
    /* 如果HTTP请求有请求体，则还需要读取 m_content_length
     * 字节消息体，状态机转移到 CHECK_STATE_CONTENT 状态 */
    if (m_content_length != 0) {
      m_check_state = CHECK_STATE_CONTENT;
      return NO_REQUEST;
    }
    /* 否则说明已经的到一个完整的HTTP请求 */
    return GET_REQUEST;
  }
  /* 处理 Connection 头部字段 */
  else if (strncasecmp(text, "Connection:", 11) == 0) {
    text += 11;
    text += strspn(text, " \t"); /* text中第一个不等于 \t 符的下标 */
    if (strcasecmp(text, "keep-alive") == 0) {
      m_linger = true;
    }
  } /* 处理 Content-Length 头部字段 */
  else if (strncasecmp(text, "Content-Length:", 15) == 0) {
    text += 15;
    text += strspn(text, " \t");
    m_content_length = atol(text);
  } /* 处理Host字段 */
  else if (strncasecmp(text, "Host:", 5) == 0) {
    text += 5;
    text += strspn(text, " \t");
    m_host = text;
  } else {
    printf("oop! unknow header %s\n", text);
  }

  return NO_REQUEST;
}

/* 解析请求体，这里没有真正的去解析，仅判断是否被完整地读入 */
http_conn::HTTP_CODE http_conn::parse_content(char *text) {
  if (m_read_idx >= (m_content_length + m_checked_idx)) {
    text[m_content_length] = '\0';
    return GET_REQUEST;
  }

  return NO_REQUEST;
}

/* 主状态机 */
http_conn::HTTP_CODE http_conn::process_read() {
  LINE_STATUS line_status = LINE_OK; /* 记录当前读取状态 */
  HTTP_CODE ret = NO_REQUEST;        /* 记录HTTP请求处理结果 */
  char *text = 0;

  while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) ||
         ((line_status = parse_line()) == LINE_OK)) {
    text = get_line();
    m_start_line = m_checked_idx;
    printf("got 1 http line: %s\n", text);

    switch (m_check_state) {
    case CHECK_STATE_REQUESTLINE: {
      ret = parse_request_line(text);
      if (ret == BAD_REQUEST) {
        return BAD_REQUEST;
      }
      break;
    }
    case CHECK_STATE_HEADER: {
      ret = parse_headers(text);
      if (ret == BAD_REQUEST) {
        return BAD_REQUEST;
      } else if (ret == GET_REQUEST) {
        return do_request();
      }
      break;
    }
    case CHECK_STATE_CONTENT: {
      ret = parse_content(text);
      if (ret == GET_REQUEST) {
        return do_request();
      }
      line_status = LINE_OPEN;
      break;
    }
    default: {
      return INTERNAL_ERROR;
    }
    }
  }

  return NO_REQUEST;
}

/* 当得到一个完整的、正确的HTTP请求时，我们就分析了目标文件的属性，如果目标文件存在，对所有用户可读，且不是目录，则使用mmap将其映射到内存地址
 * m_file_address 处，并告诉调用者获取文件成功 */
http_conn::HTTP_CODE http_conn::do_request() {
  strcpy(m_real_file, doc_root);
  int len = strlen(doc_root);
  strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
  if (stat(m_real_file, &m_file_stat) < 0) /* stat()函数获取文件信息 */
  {
    return NO_RESOURCE;
  }

  if (!(m_file_stat.st_mode & S_IROTH)) {
    return FORBIDDEN_REQUEST;
  }

  if (S_ISDIR(m_file_stat.st_mode)) {
    return BAD_REQUEST;
  }

  int fd = open(m_real_file, O_RDONLY);
  m_file_address =
      (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return FILE_REQUEST;
}

/* 对内存映射区执行 munmap操作 */
void http_conn::unmap() {
  if (m_file_address) {
    munmap(m_file_address, m_file_stat.st_size);
    m_file_address = 0;
  }
}

/* 写HTTP 响应 */
bool http_conn::write() {
  int temp = 0;
  int bytes_have_send = 0;
  int bytes_to_send = m_write_idx;
  if (bytes_to_send == 0) {
    modfd(m_epollfd, m_sockfd, EPOLLIN);
    init();
    return true;
  }

  while (1) {
    temp = writev(m_sockfd, m_iv, m_iv_count);
    if (temp <= -1) {
      /* 如果TCP*
       * 写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间，服务器无法立即接收到同一客户端的下一个请求，但这样可以保证连接的完整性*/
      if (errno == EAGAIN) {
        modfd(m_epollfd, m_sockfd, EPOLLOUT);
        return true;
      }
      unmap();
      return false;
    }

    bytes_to_send -= temp;
    bytes_have_send += temp;
    /* 发送HTTP响应成功，根据HTTP请求中的Contention 字段决定是否理解关闭连接
     */
    if (bytes_to_send <= bytes_have_send) {
      unmap();
      if (m_linger) {
        init();
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return true;
      } else {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return false;
      }
    }
  }
}

/* 往写缓冲中写入待发送的数据 */
bool http_conn::add_response(const char *format, ...) {
  if (m_write_idx >= WRITE_BUFFER_SIZE) {
    return false;
  }
  /* VA_LIST 是C语言的宏解决可变参数的问题 */
  va_list arg_list;
  va_start(arg_list, format);
  /* vsnprintf()用于向一个字符串缓冲区打印格式化字符串，且可以限定打印的格式化字符串的最大长度。
   */
  int len = vsnprintf(m_write_buf + m_write_idx,
                      WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
  if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)) {
    return false;
  }
  m_write_idx += len;
  va_end(arg_list);
  return true;
}

bool http_conn::add_status_line(int status, const char *title) {
  return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len) {
  add_content_length(content_len);
  add_linger();
  add_blank_line();
}

bool http_conn::add_content_length(int content_len) {
  return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_linger() {
  return add_response("Connection: %s\r\n",
                      (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line() { return add_response("%s", "\r\n"); }

bool http_conn::add_content(const char *content) {
  return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret) {
  switch (ret) {
  case INTERNAL_ERROR: {
    add_status_line(500, error_500_title);
    add_headers(strlen(error_500_form));
    if (!add_content(error_500_form)) {
      return false;
    }
    break;
  }
  case BAD_REQUEST: {
    add_status_line(400, error_400_title);
    add_headers(strlen(error_400_form));
    if (!add_content(error_400_form)) {
      return false;
    }
    break;
  }
  case NO_RESOURCE: {
    add_status_line(404, error_404_title);
    add_headers(strlen(error_404_form));
    if (!add_content(error_404_form)) {
      return false;
    }
    break;
  }
  case FORBIDDEN_REQUEST: {
    add_status_line(403, error_403_title);
    add_headers(strlen(error_403_form));
    if (!add_content(error_403_form)) {
      return false;
    }
    break;
  }
  case FILE_REQUEST: {
    add_status_line(200, ok_200_title);
    if (m_file_stat.st_size != 0) {
      add_headers(m_file_stat.st_size);
      m_iv[0].iov_base = m_write_buf;
      m_iv[0].iov_len = m_write_idx;
      m_iv[1].iov_base = m_file_address;
      m_iv[1].iov_len = m_file_stat.st_size;
      m_iv_count = 2;
      return true;
    } else {
      const char *ok_string = "<html><body></body></html>";
      add_headers(strlen(ok_string));
      if (!add_content(ok_string)) {
        return false;
      }
    }
  }
  default: {
    return false;
  }
  }

  m_iv[0].iov_base = m_write_buf;
  m_iv[0].iov_len = m_write_idx;
  m_iv_count = 1;
  return true;
}

/* 由线程池中的工作线程调用，这是处理 HTTP 请求的入口函数 */
void http_conn::process() {
  HTTP_CODE read_ret = process_read();
  if (read_ret == NO_REQUEST) {
    /* EPOLLIN事件则只有当对端有数据写入时才会触发，所以触发一次后需要不断读取所有数据直到读完EAGAIN为止。否则剩下的数据只有在下次对端有写入时才能一起取出来了。
     */
    modfd(m_epollfd, m_sockfd, EPOLLIN);
    return;
  }

  bool write_ret = process_write(read_ret);
  if (!write_ret) {
    close_conn();
  }
  /* EPOLLOUT事件只有在不可写到可写的转变时刻，才会触发一次 */
  modfd(m_epollfd, m_sockfd, EPOLLOUT);
}