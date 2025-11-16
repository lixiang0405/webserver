#pragma once
#include "RouterHandler.h"
#include "YiyiCatServer.h"

class AiGameStartHandler : public RouterHandler
{
public:
    explicit AiGameStartHandler(YiyiCatServer* server) : server_(server) {}

    void handle(const HttpRequest& req, HttpResponse* resp) override;

private:
    YiyiCatServer* server_;
};