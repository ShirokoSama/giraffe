//
// Created by Srf on 2018/12/18.
// This is a error-handling wrapper header contains wrapped system calls and error msg log to stderr
// like csapp.h and csapp.c
//

#ifndef HTTPSERVER_SYSTEM_H
#define HTTPSERVER_SYSTEM_H

#include <string>
using std::string;

// system calls from c headers
#include <unistd.h>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <arpa/inet.h>

void unix_error(const string &msg);
void posix_error(int code, const string &msg);
void gai_error(int code, const string &msg);
void app_error(const string &msg);

int Open(const char *pathname, int flags, mode_t mode);
int Close(int fd);

// robust io functions from csapp.h
#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* Descriptor for this internal buf */
    size_t rio_cnt;               /* Unread bytes in internal buf */
    char *rio_bufptr;          /* Next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* Internal buffer */
} rio_t;
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, const void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd);
ssize_t	rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t	rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
// relative wrappers
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);
void Rio_writen(int fd, const void *usrbuf, size_t n);
void Rio_readinitb(rio_t *rp, int fd);
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

int Fcntl(int fd, int cmd, int flag);

typedef void handler_t(int);
handler_t *Signal(int signum, handler_t handler);

/* Memory mapping wrappers */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void Munmap(void *start, size_t length);

// socket functions
#define LISTENQ  1024
int GetAddrInfo(const char *host, const char *service, const struct addrinfo *hints, struct addrinfo **result);
void FreeAddrInfo(struct addrinfo *ai);
int SetSockOpt(int sockFD, int level, int optName, const void *optVal, socklen_t optLen);
int Socket(int domain, int type, int protocol);
void Bind(int sockFD, struct sockaddr *my_addr, socklen_t addrLen);
void Listen(int s, int backlog);
int Accept(int s, struct sockaddr *addr, socklen_t *addrLen);
void Connect(int sockFD, struct sockaddr *service_addr, socklen_t addrLen);

#ifdef __linux__

#include <sys/epoll.h>

int EpollCreate();
int EpollCtl(int epfd, int op, int fd, struct epoll_event *event);
int EpollWait(int epfd, struct epoll_event *events, int max_events, int timeout);

#else

#include <sys/select.h>

int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

#endif

#endif //HTTPSERVER_SYSTEM_H
