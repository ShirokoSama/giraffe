//
// Created by Srf on 2018/12/18.
//

#ifndef HTTPSERVER_ARGS_PARSER_H
#define HTTPSERVER_ARGS_PARSER_H

#include <string>
#include <unordered_map>
#include <iostream>

class ArgsTokenizer {
public:
    ArgsTokenizer() = default;
    ~ArgsTokenizer() = default;

    void Parse(int argc, char *argv[]) {
        for (int i = 1; i < argc; i++) {
            std::string argKey(argv[i]);
            if (argKey.find('-') != std::string::npos && i + 1 < argc) {
                std::string argValue(argv[i + 1]);
                if (argValue.find('-') == std::string::npos) {
                    m_argsMap.insert(std::make_pair(argKey, argValue));
                    i++;
                }
                else
                    m_argsMap.insert(std::make_pair(argKey, ""));
            }
        }
    }

    bool FindArg(const std::string &argKey, std::string *argValue) {
        std::unordered_map<std::string, std::string>::iterator iter;
        if ((iter = m_argsMap.find(argKey)) != m_argsMap.end()) {
            *argValue = iter->second;
            return true;
        }
        else
            return false;
    }

private:
    std::unordered_map<std::string, std::string> m_argsMap;
};

#endif //HTTPSERVER_ARGS_PARSER_H
