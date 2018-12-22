//
// Created by Srf on 2018/12/19.
//

#ifndef HTTPSERVER_HTTP_SERVER_H
#define HTTPSERVER_HTTP_SERVER_H

#include <ctime>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <regex>
#include <unordered_map>

#include "system.h"
#include "ext/concurrentqueue.h"
#include "ext/zlib/zlib.h"

using std::vector;
using std::pair;
using std::string;
using std::cout;
using std::endl;
using std::time_t;

#define MAX_CONSUMER_NUM 10
#define KEEP_ALIVE_TIMEOUT 5
#define GZIP_OPEN true

struct RequestBuffer {
    int epfd;
    int fd;
    string method;
    string uri;
    string protocol;
    int contentLen;
    string contentType;
    string body;
    string tempBuf;
};

typedef moodycamel::ConcurrentQueue<RequestBuffer> ConcurrentQueue;
typedef vector<pair<string, string>> params;

namespace http_tool {
    // pre compiled regex
    const std::regex r_extension("(.*)\\.(.*)$");
    const std::regex r_contentLen("content-length:\\s*(.+?)[\r\n]", std::regex::icase);
    const std::regex r_contentType("content-type:\\s*(.+?)[\r\n]", std::regex::icase);
    const std::regex r_body("(\r\n\r\n|\n\n)(.*)");
    const std::regex r_param("^(.+?)=(.+?)(&|\\s*$)");

    extern std::unordered_map<int, string> code_msg_map;

    bool checkRequest(RequestBuffer &rb);
    string contentType(const string &fileName);
    void sendError(int socket_fd, int status_code, const string &base_dir);
    bool parseParams(const string &str, params &list);
    bool exec(const string &command, string &result);
    string urlDecode(const string &str);
    int gzCompress(Bytef *data, uInt n_data, Bytef *gz_data, uInt *n_gz_data);
}

#endif //HTTPSERVER_HTTP_SERVER_H