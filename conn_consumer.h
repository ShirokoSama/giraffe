//
// Created by Srf on 2018/12/19.
//

#ifndef HTTPSERVER_CONN_CONSUMER_H
#define HTTPSERVER_CONN_CONSUMER_H

#include "system.h"
#include "http_server.h"

class ConnConsumer {
public:
    explicit ConnConsumer(ConcurrentQueue *queue, const string &dir);
    ~ConnConsumer() = default;
    void Execute();

private:
    string baseDir;
    ConcurrentQueue *c_queue;

    void response(const RequestBuffer &rb);
    void processGET(const RequestBuffer &rb);
    void processPOST(const RequestBuffer &rb);
    void processError(int epoll_fd, int socket_fd, int error_code);
};

#endif //HTTPSERVER_CONN_CONSUMER_H
