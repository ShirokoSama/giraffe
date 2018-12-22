//
// Created by Srf on 2018/12/18.
//

#include "conn_producer.h"

int OpenListenFD(const char *port) {
    int listenFD = -1, optValue = 1;
    struct addrinfo hints = {0, 0, 0, 0, 0, nullptr, nullptr, nullptr};
    struct addrinfo *pList;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
    GetAddrInfo(nullptr, port, &hints, &pList);

    auto p = pList;
    for (p = pList; p; p = p->ai_next) {
        if ((listenFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; // fail, try next
        SetSockOpt(listenFD, SOL_SOCKET, SO_REUSEADDR, (const void *)&optValue, sizeof(int));
        if (bind(listenFD, p->ai_addr, p->ai_addrlen) == 0)
            break; // success
        Close(listenFD);
    }

    FreeAddrInfo(pList);
    if (!p) return -1; // No address worked
    // avoid memory leak
    if (listen(listenFD, LISTENQ) < 0) {
        Close(listenFD);
        return -1;
    }
    return listenFD;
}

ConnProducer::ConnProducer(int listenFD, ConcurrentQueue *queue, const string &baseDir) {
    this->listen_fd = listenFD;
    this->queue = queue;
    this->baseDir = baseDir;
    this->request_map = unordered_map<int, RequestBuffer>();
    this->conn_poll = unordered_map<int, time_t>();
#ifdef __linux__
    // reference -- http://man7.org/linux/man-pages/man7/epoll.7.html
    this->epoll_fd = EpollCreate();
    this->event.events = EPOLLIN;
    this->event.data.fd = listen_fd;
    EpollCtl(this->epoll_fd, EPOLL_CTL_ADD, this->listen_fd, &this->event);
#else
    FD_ZERO(&reads);
    FD_SET(this->listen_fd, &reads);
    fd_max = this->listen_fd;
#endif
}

void ConnProducer::Execute() {
#ifdef __linux__
    struct sockaddr_in client_address{};
    char buf[BUF_SIZE];
    while (true) {
        int event_count = EpollWait(epoll_fd, epoll_events, MAX_EPOLL_EVENTS, 5);
        if (event_count > 0) {
            for (int i = 0; i < event_count; i++) {
                if (epoll_events[i].data.fd == listen_fd) {
                    socklen_t address_len = sizeof(client_address);
                    int conn_socket = Accept(listen_fd, (struct sockaddr *)&client_address, &address_len);
                    // add socket fd and current timestamp into conn_pool
                    conn_poll.insert(std::make_pair(conn_socket, std::time(nullptr)));
                    // set non blocking
                    int flag = Fcntl(conn_socket, F_GETFL, 0);
                    Fcntl(conn_socket, F_SETFL, flag|O_NONBLOCK);
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = conn_socket;
                    EpollCtl(epoll_fd, EPOLL_CTL_ADD, conn_socket, &event);
                    printf("Connection Request from: %s:%d and conn socket fd is %d\n",
                           inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), conn_socket);
                }
                else {
                    buildRequest(epoll_events[i].data.fd);
                }
            }
        }
        // no new connect and start clear out of time connection
        else {
            for (auto itr = conn_poll.begin(); itr != conn_poll.end();) {
                if (std::time(nullptr) - itr->second > KEEP_ALIVE_TIMEOUT) {
                    EpollCtl(this->epoll_fd, EPOLL_CTL_DEL, itr->first, nullptr);
                    close(itr->first);
                    cout << "delete conn " << itr->first << endl;
                    itr = conn_poll.erase(itr);
                }
                else
                    itr++;
            }
        }
    }
#else
    int fd_num;
    struct sockaddr_in client_address{};
    char buf[BUF_SIZE];
    while (true) {
        cpy_reads = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 5000;
    }
    if ((fd_num = Select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == 0)
        continue;
    for (int i = 0; i < fd_max + 1; i++) {
        if (FD_ISSET(i, &cpy_reads)) {
            if (i == this->listen_fd) {
                socklen_t address_len = sizeof(client_address);
                int conn_socket = Accept(listen_fd, (struct sockaddr *)&client_address, &address_len);
                FD_SET(conn_socket, &reads);
                if (fd_max < conn_socket)
                    fd_max = connect_socket;
                printf("Connection Request from: %s:%d and conn socket fd is %d\n",
                           inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), conn_socket);
            }
            else {
                buildRequest(i);
            }
        }
    }
#endif
}

void ConnProducer::Drop() {
#ifdef __linux__
    Close(epoll_fd);
#endif
    Close(listen_fd);
}

void ConnProducer::buildRequest(int fd) {
    if (request_map.find(fd) == request_map.end()) {
#ifdef __linux__
        RequestBuffer rb{this->epoll_fd, fd, "", "", "", 0, "", "", ""};
#else
        RequestBuffer rb{0, fd, "", "", "", 0, "", "", ""};
#endif
        request_map.insert(std::make_pair(fd, rb));
    }
    RequestBuffer &rb = request_map[fd];
    ssize_t str_len;

#ifdef __linux__
    // non-block read loop in et epoll
    while (true) {
        char buf[BUF_SIZE];
        str_len = read(fd, buf, BUF_SIZE);
        if (str_len > 0)
            rb.tempBuf += string(buf, static_cast<unsigned long>(str_len));
        else
            break;
    }
#else
    str_len = read(i, buf, BUF_SIZE);
    if (str_len > 0)
        rb.tempBuf += string(buf, static_cast<unsigned long>(str_len));
#endif

    if (http_tool::checkRequest(rb)) {
        rb.tempBuf.clear();
        queue->enqueue(rb);
        request_map.erase(fd);
        if (conn_poll.find(rb.fd) != conn_poll.end())
            conn_poll[rb.fd] = std::time(nullptr);
        else
            conn_poll.insert(std::make_pair(rb.fd, std::time(nullptr)));
    }
    // eof or invalid non-blocking read error
    else if (str_len == 0 || (str_len < 0 && errno != EAGAIN)) {
        http_tool::sendError(fd, 400, this->baseDir);
#ifdef __linux__
        EpollCtl(this->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
#endif
        Close(fd);
        printf("Closed client: %d\n", fd);
    }
    else {
        if (rb.contentLen == 0) {
            http_tool::sendError(fd, 400, this->baseDir);
#ifdef __linux__
            EpollCtl(this->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
#endif
            Close(fd);
            printf("Closed client: %d\n", fd);
        }
    }

}
