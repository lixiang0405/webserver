#pragma once
#include "RouterHandler.h"
#include "YiyiCatServer.h"

class GameBackendHandler : public RouterHandler 
{
public:
    explicit GameBackendHandler(YiyiCatServer* server) : server_(server) {}

    void handle(const HttpRequest& req, HttpResponse* resp) override;

private:
    YiyiCatServer* server_;
};