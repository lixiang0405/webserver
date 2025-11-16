#pragma once
#include "RouterHandler.h"
#include "MysqlUtil.h"
#include "YiyiCatServer.h"
#include "JsonUtil.h"


class LoginHandler : public RouterHandler 
{
public:
    explicit LoginHandler(YiyiCatServer* server) : server_(server) {}
    
    void handle(const HttpRequest& req, HttpResponse* resp) override;

private:
    int queryUserId(const std::string& username, const std::string& password);

private:
    YiyiCatServer*       server_;
    MysqlUtil     mysqlUtil_;
};