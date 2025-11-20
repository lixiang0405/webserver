#pragma once
#include "RouterHandler.h"
#include "MysqlUtil.h"
#include "YiyiCatServer.h"
#include "JsonUtil.h"
#include<functional>


class LoginHandler : public RouterHandler 
{
public:
    using UnLoginCallBack = std::function<void(const HttpRequest&, HttpResponse*, int)>;
    explicit LoginHandler(YiyiCatServer* server) : server_(server), unLoginCallBack_(nullptr) {}
    
    void handle(const HttpRequest& req, HttpResponse* resp) override;

    void setUnLoginCallBack(const UnLoginCallBack &unLoginCallBack){
        unLoginCallBack_ = unLoginCallBack;
    }
private:
    int queryUserId(const std::string& username, const std::string& password);

private:
    YiyiCatServer*       server_;
    MysqlUtil     mysqlUtil_;
    UnLoginCallBack unLoginCallBack_;
};