//
// Created by Srf on 2018/12/20.
//

#include "http_server.h"

bool http_tool::checkRequest(RequestBuffer &rb) {
    if (rb.contentLen == 0) {
        if (rb.tempBuf.find("\r\n\r\n") != string::npos || rb.tempBuf.find("\n\n") != string::npos) {
            std::istringstream ss(rb.tempBuf);
            ss >> rb.method;
            ss >> rb.uri;
            ss >> rb.protocol;
            std::smatch match;
            if (std::regex_search(rb.tempBuf, match, r_contentType))
                rb.contentType = match.format("$1");
            // start check content length and body
            if (std::regex_search(rb.tempBuf, match, r_contentLen)) {
                int len = std::stoi(match.format("$1"));
                if (len > 0) {
                    rb.contentLen = len;
                    if (std::regex_search(rb.tempBuf, match, r_body)) {
                        if (match.format("$2").length() < rb.contentLen)
                            return false;
                        else
                            rb.body = match.format("$2");
                    }
                }
            }
        }
        else
            return false;
    }
    else {
        // start check content length and body
        std::smatch match;
        if (std::regex_search(rb.tempBuf, match, r_body)) {
            if (match.format("$2").length() < rb.contentLen)
                return false;
            else
                rb.body = string(match.format("$2"));
        }
    }
    return true;
}

string http_tool::contentType(const string &fileName) {
    std::smatch match;
    if (std::regex_search(fileName, match, r_extension)) {
        string extension = match.format("$2");
        if (extension == "html" || extension == "htm")
            return "text/html";
        else if (extension == "css")
            return "text/css";
        else if (extension == "csv")
            return "text/csv";
        else if (extension == "xml")
            return "text/xml";
        else if (extension == "png")
            return "image/png";
        else if (extension == "ico")
            return "image/x-icon";
        else if (extension == "gif")
            return "image/gif";
        else if (extension == "jpg" || extension == "jpeg")
            return "image/jpeg";
    }
    return "text/plain";
}

std::unordered_map<int, string> http_tool::code_msg_map = {
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"}
};

void http_tool::sendError(int socket_fd, int status_code, const string &base_dir) {
    std::ostringstream ss;
    ss << base_dir << "/" << status_code << ".html";
    string errorPageName = ss.str();
    struct stat fileInfo;
    if (stat(errorPageName.c_str(), &fileInfo) < 0)
        unix_error("Cannot find correspond error page!");
    if (code_msg_map.find(status_code) == code_msg_map.end())
        app_error("Cannot find correspond error code!");
    size_t fileSize = static_cast<size_t >(fileInfo.st_size);
    ss = std::ostringstream();
    ss << "HTTP/1.1 " << status_code << " " << code_msg_map[status_code] << "\r\n" << "Connection: close\r\n"
        << "Content type: text/html\r\nContent-length: " << fileSize << "\r\nServer: giraffe\r\n\r\n";
    string responseHeader = ss.str();
    Rio_writen(socket_fd, responseHeader.c_str(), responseHeader.size());
    // use mmap as a char buffer
    int srcfd = Open(errorPageName.c_str(), O_RDONLY, 0);
    char *srcp = static_cast<char *>(Mmap(0, fileSize, PROT_READ, MAP_PRIVATE, srcfd, 0));
    Close(srcfd);
    Rio_writen(socket_fd, srcp, fileSize);
    Munmap(srcp, fileSize);
}

bool http_tool::parseParams(const string &str, params &list) {
    bool valid = true;
    std::smatch match;
    string paramStr(str);
    while (std::regex_search(paramStr, match, r_param)) {
        list.push_back(pair<string, string>(match.format("$1"), match.format("$2")));
        paramStr = match.suffix().str();
    }
    if (!paramStr.empty())
        valid = false;
    return valid;
}

bool http_tool::exec(const string &command, string &result) {
    char buf[512];
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) return false;
    try {
        while (fgets(buf, sizeof(buf), pipe) != nullptr)
            result += string(buf);
    } catch (...) {
        pclose(pipe);
        return false;
    }
    pclose(pipe);
    return true;
}

string http_tool::urlDecode(const string &str){
    string ret;
    char ch;
    int ii;
    unsigned long i, len = str.size();
    for (i=0; i < len; i++) {
        if (str[i] != '%') {
            if (str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        } else {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}

// this code is from internet!!
// I'm not sure whether this is the correct way to use zlib
int http_tool::gzCompress(Bytef *data, uInt n_data, Bytef *gz_data, uInt *n_gz_data) {
    z_stream c_stream;
    int err = 0;
    if(data && n_data > 0) {
        c_stream.zalloc = NULL;
        c_stream.zfree = NULL;
        c_stream.opaque = NULL;
        //只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
        if(deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                        MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) return -1;
        c_stream.next_in  = data;
        c_stream.avail_in  = n_data;
        c_stream.next_out = gz_data;
        c_stream.avail_out  = *n_gz_data;
        while(c_stream.avail_in != 0 && c_stream.total_out < *n_gz_data) {
            if(deflate(&c_stream, Z_NO_FLUSH) != Z_OK) return -1;
        }
        if(c_stream.avail_in != 0) return c_stream.avail_in;
        for(;;) {
            if((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) break;
            if(err != Z_OK) return -1;
        }
        if(deflateEnd(&c_stream) != Z_OK) return -1;
        *n_gz_data = static_cast<uInt>(c_stream.total_out);
        return 0;
    }
    return -1;
}
