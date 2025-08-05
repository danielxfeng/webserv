
#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class WebServ
{
private:
    // Config config_;
    std::unordered_map<unsigned int, std::string> serverMap_; // todo: implement Server class
public:
    WebServ() = default;
    ~WebServ() = default;
    void eventLoop();
};
