//
// Created by Srf on 2018/12/19.
//

#include "conn_consumer.h"

ConnConsumer::ConnConsumer(ConcurrentQueue *queue, const string &dir) {
    this->c_queue = queue;
    this->baseDir = dir;
}

void ConnConsumer::Execute() {
    while (true) {
        RequestBuffer rb;
        if (c_queue->try_dequeue(rb)) {
            rb.uri = http_tool::urlDecode(rb.uri);
            rb.body = http_tool::urlDecode(rb.body);
            this->response(rb);
        }
    }
}

void ConnConsumer::response(const RequestBuffer &rb) {
    cout << rb.fd << " " << rb.method << " " << rb.uri << " " << rb.protocol << " " << rb.contentType
              << " " << rb.contentLen << " " << rb.body << endl;
    if (rb.method == "GET" || rb.method == "HEAD")
        processGET(rb);
    else if (rb.method == "POST")
        processPOST(rb);
    else
        processError(rb.epfd, rb.fd, 501);
}

void ConnConsumer::processGET(const RequestBuffer &rb) {
    string filePath = baseDir + rb.uri;
    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) < 0) {
        processError(rb.epfd, rb.fd, 404);
        return;
    }
    // is a regular file or is owner having read permission
    if (!(S_ISREG(fileInfo.st_mode)) || !(S_IRUSR & fileInfo.st_mode)) {
        processError(rb.epfd, rb.fd, 403);
        return;
    }
    // start build response content
    size_t fileSize = static_cast<size_t >(fileInfo.st_size);
    // use mmap as a char buffer
    if (rb.method == "GET") {
        int srcfd = Open(filePath.c_str(), O_RDONLY, 0);
        char *srcp = static_cast<char *>(Mmap(0, fileSize, PROT_READ, MAP_PRIVATE, srcfd, 0));
        Close(srcfd);
        if (GZIP_OPEN) {
            Bytef gz_data[fileSize];
            uInt gz_Size = static_cast<uInt>(fileSize);
            int rc = http_tool::gzCompress(reinterpret_cast<Bytef *>(srcp), static_cast<uInt>(fileSize), gz_data, &gz_Size);
            std::ostringstream ss;
            ss << "HTTP/1.1 200 OK\r\nServer: giraffe\r\nContent-type: " << http_tool::contentType(rb.uri)
               << "\r\nContent-length: " << gz_Size << "\r\nContent-Encoding: gzip\r\n\r\n";
            string responseHeader = ss.str();
            Rio_writen(rb.fd, responseHeader.c_str(), responseHeader.size());
            Rio_writen(rb.fd, gz_data, gz_Size);
            Munmap(srcp, fileSize);
        }
        else {
            std::ostringstream ss;
            ss << "HTTP/1.1 200 OK\r\nServer: giraffe\r\nContent-type: " << http_tool::contentType(rb.uri)
               << "\r\nContent-length: " << fileSize << "\r\n\r\n";
            string responseHeader = ss.str();
            Rio_writen(rb.fd, responseHeader.c_str(), responseHeader.size());
            Rio_writen(rb.fd, srcp, fileSize);
            Munmap(srcp, fileSize);
        }
    }
}

void ConnConsumer::processPOST(const RequestBuffer &rb) {
    if (rb.contentType != "application/x-www-form-urlencoded") {
        http_tool::sendError(rb.fd, 501, baseDir);
        return;
    }
    string programPath = baseDir + rb.uri;
    params pList;
    unsigned long pos;
    if (rb.contentLen > 0) {
        if (!http_tool::parseParams(rb.body, pList)) {
            processError(rb.epfd, rb.fd, 400);
            return;
        }
    }
    else if ((pos = rb.uri.find('?')) != string::npos) {
        programPath = rb.uri.substr(0, pos);
        if (!http_tool::parseParams(rb.uri.substr(pos + 1), pList)) {
            processError(rb.epfd, rb.fd, 400);
            return;
        }
    }

    struct stat programInfo;
    if (stat(programPath.c_str(), &programInfo) < 0) {
        processError(rb.epfd, rb.fd, 404);
        return;
    }
    if (!(S_ISREG(programInfo.st_mode)) || !(S_IXUSR & programInfo.st_mode)) {
        processError(rb.epfd, rb.fd, 403);
        return;
    }

    string command(programPath);
    for (auto p: pList) {
        // log parsed params
        cout << p.first << ": " << p.second << endl;
        command += (" " + p.second);
    }
    string result;
    cout << command << endl;
    if (!http_tool::exec(command, result)) {
        processError(rb.epfd, rb.fd, 500);
    }
    std::ostringstream ss;
    ss << "HTTP/1.1 200 OK\r\nServer: giraffe\r\nContent-type: text/plain\r\nContent-length: "
       << result.size() << "\r\n\r\n";
    string responseHeader = ss.str();
    Rio_writen(rb.fd, responseHeader.c_str(), responseHeader.size());
    Rio_writen(rb.fd, result.c_str(), result.size());
}

void ConnConsumer::processError(int epoll_fd, int socket_fd, int error_code) {
    http_tool::sendError(socket_fd, error_code, this->baseDir);
#ifdef __linux__
    EpollCtl(epoll_fd, EPOLL_CTL_DEL, socket_fd, nullptr);
#endif
    Close(socket_fd);
    printf("Closed client: %d\n", socket_fd);
}
