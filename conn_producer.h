//
// Created by Srf on 2018/12/18.
//

#ifndef HTTPSERVER_CONN_PRODUCER_H
#define HTTPSERVER_CONN_PRODUCER_H

#include "system.h"
#include "http_server.h"

int OpenListenFD(const char *port);

#define BUF_SIZE 1024
#ifdef __linux__
#define MAX_EPOLL_EVENTS 50
#endif

using std::unordered_map;

class ConnProducer {
public:
    explicit ConnProducer(int listenFD, ConcurrentQueue *queue, const string &baseDir);
    ~ConnProducer() = default;
    void Execute();
    void Drop();

private:
    int listen_fd;
    unordered_map<int, RequestBuffer> request_map;
    unordered_map<int, time_t> conn_poll;
    ConcurrentQueue *queue;
    string baseDir;

#ifdef __linux__
    int epoll_fd;
    struct epoll_event epoll_events[MAX_EPOLL_EVENTS];
    struct epoll_event event;
#else
    fd_set reads, cpy_reads;
    struct timeval timeout;
    int fd_max;
#endif

    void buildRequest(int fd);
};

#endif //HTTPSERVER_CONN_PRODUCER_H