#pragma once
#include "RouterHandler.h"
#include "YiyiCatServer.h"
#include "JsonUtil.h"

class LogoutHandler : public RouterHandler 
{
public:
    explicit LogoutHandler(YiyiCatServer* server) : server_(server) {}
    void handle(const HttpRequest& req, HttpResponse* resp) override;
private:
    YiyiCatServer* server_;
};