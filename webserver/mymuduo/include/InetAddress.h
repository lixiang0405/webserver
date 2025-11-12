#pragma once

#include<string>
#include<arpa/inet.h>
#include<netinet/in.h>

class InetAddress
{
private:
    sockaddr_in addr_;
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "0.0.0.0");
    explicit InetAddress(const sockaddr_in& addr) : addr_(addr){}

    std::string toIp()const;
    std::string toIpPort()const;
    uint16_t toPort()const;

    const sockaddr_in *getSock() const{
        return &addr_;
    }
    void setSock(sockaddr_in addr){
        addr_ = addr;
    }
};

