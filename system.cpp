//
// Created by Srf on 2018/12/18.
//

#include "system.h"

/* Unix-style error */
void unix_error(const string &msg) {
    fprintf(stderr, "%s: %s\n", msg.c_str(), strerror(errno));
    exit(EXIT_FAILURE);
}

/* Posix-style error */
void posix_error(int code, const string &msg) {
    fprintf(stderr, "%s: %s\n", msg.c_str(), strerror(code));
    exit(EXIT_FAILURE);
}

/* Getaddrinfo-style error */
void gai_error(int code, const string &msg) {
    fprintf(stderr, "%s: %s\n", msg.c_str(), gai_strerror(code));
    exit(EXIT_FAILURE);
}

/* Application error */
void app_error(const string &msg) {
    fprintf(stderr, "%s\n", msg.c_str());
    exit(EXIT_FAILURE);
}

/****************************************
 * The Rio package - Robust I/O functions
 ****************************************/

/*
 * rio_readn - Robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufp = static_cast<char *>(usrbuf);
    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR) /* Interrupted by sig handler return */
                nread = 0;      /* and call read() again */
            else
                return -1;      /* errno set by read() */
        }
        else if (nread == 0)
            break;              /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);         /* Return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - Robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, const void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufp = static_cast<const char *>(usrbuf);
    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
#ifdef __linux__
            // important!!
            // because of using non blocking fd so we must judge EAGAIN errno and try loop
            if (errno == EAGAIN)
                continue;
#endif
            if (errno == EINTR)  /* Interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else
                return -1;       /* errno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */

/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
    size_t cnt;
    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */
        rp->rio_cnt = static_cast<size_t>(read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf)));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) /* Interrupted by sig handler return */
                return -1;
        }
        else if (rp->rio_cnt == 0)  /* EOF */
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }
    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufp = static_cast<char *>(usrbuf);
    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1;          /* errno set by read() */
        else if (nread == 0)
            break;              /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/*
 * rio_readlineb - Robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = static_cast<char *>(usrbuf);
    for (n = 1; n < maxlen; n++) {
        if ((rc = static_cast<int>(rio_read(rp, &c, 1))) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                n++;
                break;
            }
        } else if (rc == 0) {
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break;    /* EOF, some data was read */
        } else
            return -1;	  /* Error */
    }
    *bufp = 0;
    return n - 1;
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) {
    ssize_t n;
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
        unix_error("Rio_readn Error");
    return n;
}

void Rio_writen(int fd, const void *usrbuf, size_t n) {
    if (rio_writen(fd, usrbuf, n) != n)
        unix_error("Rio_writen Error");
}

void Rio_readinitb(rio_t *rp, int fd) {
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
    ssize_t rc;
    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error("Rio_readnb Error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t rc;
    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error("Rio_readlineb error");
    return rc;
}

int Fcntl(int fd, int cmd, int flag) {
    int rc;
    if ((rc = fcntl(fd, cmd, flag)) < 0)
        unix_error("Fcntl Error");
    return rc;
}

handler_t *Signal(int signum, handler_t handler) {
    struct sigaction action{}, old_action{};
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); // Block signals of type being handled
    action.sa_flags = SA_RESTART; // Restart system calls if possible
    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal Error");
    return (old_action.sa_handler);
}

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
    void *ptr;
    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
        unix_error("mmap error");
    return(ptr);
}

void Munmap(void *start, size_t length) {
    if (munmap(start, length) < 0)
        unix_error("munmap error");
}

int GetAddrInfo(const char *host, const char *service, const struct addrinfo *hints, struct addrinfo **result) {
    int rc;
    if ((rc = getaddrinfo(host, service, hints, result)) != 0)
        gai_error(rc, "GetAddrInfo Error");
    return rc;
}

void FreeAddrInfo(struct addrinfo *ai) {
    freeaddrinfo(ai);
}

int SetSockOpt(int sockFD, int level, int optName, const void *optVal, socklen_t optLen) {
    int rc;
    if ((rc = setsockopt(sockFD, level, optName, optVal, optLen)) < 0)
        unix_error("SetSockOpt Error");
    return rc;
}

int Socket(int domain, int type, int protocol) {
    int rc;
    if ((rc = socket(domain, type, protocol)) < 0)
        unix_error("Socket Error");
    return rc;
}

void Bind(int sockFD, struct sockaddr *my_addr, socklen_t addrLen) {
    int rc;
    if ((rc = bind(sockFD, my_addr, addrLen)) < 0)
        unix_error("Bind Error");
}

void Listen(int s, int backlog) {
    int rc;
    if ((rc = listen(s,  backlog)) < 0)
        unix_error("Listen Error");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrLen) {
    int rc;
    if ((rc = accept(s, addr, addrLen)) < 0)
        unix_error("Accept Error");
    return rc;
}

void Connect(int sockFD, struct sockaddr *service_addr, socklen_t addrLen) {
    int rc;
    if ((rc = connect(sockFD, service_addr, addrLen)) < 0)
        unix_error("Connect Error");
}

int Open(const char *pathname, int flags, mode_t mode) {
    int rc;
    if ((rc = open(pathname, flags, mode))  < 0)
        unix_error("Open Error");
    return rc;
}

int Close(int fd) {
    int rc;
    if ((rc = close(fd)) < 0)
        unix_error("Close Error");
    return rc;
}

#ifdef __linux__

int EpollCreate() {
    int rc;
    // use epoll_create1 instead of epoll_create to avoid obsolete size argument
    if ((rc = epoll_create1(EPOLL_CLOEXEC)) < 0)
        unix_error("Epoll Create Error");
    return rc;
}

int EpollCtl(int epfd, int op, int fd, struct epoll_event *event) {
    int rc;
    if ((rc = epoll_ctl(epfd, op, fd, event)) < 0)
        unix_error("Epoll Ctl Error");
    return rc;
}

int EpollWait(int epfd, struct epoll_event *events, int max_events, int timeout) {
    int rc;
    if ((rc = epoll_wait(epfd, events, max_events, timeout)) < 0)
        unix_error("Epoll Wait Error");
    return rc;
}

#else

int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    int rc;
    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
        unix_error("Select Error");
    return rc;
}

#endif