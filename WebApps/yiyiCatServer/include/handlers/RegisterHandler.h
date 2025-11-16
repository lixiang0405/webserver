#pragma once
#include "RouterHandler.h"
#include "MysqlUtil.h"
#include "YiyiCatServer.h"

class RegisterHandler : public RouterHandler 
{
public:
    explicit RegisterHandler(YiyiCatServer* server) : server_(server) {}

    void handle(const HttpRequest& req, HttpResponse* resp) override;
private:
    int insertUser(const std::string& username, const std::string& password);
    bool isUserExist(const std::string& username);
private:
    YiyiCatServer* server_;
    MysqlUtil     mysqlUtil_;
};