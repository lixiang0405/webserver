#pragma once
#include "RouterHandler.h"
#include "YiyiCatServer.h"

class AiGameMoveHandler : public RouterHandler
{
public:
    explicit AiGameMoveHandler(YiyiCatServer* server) : server_(server) {}
    void handle(const HttpRequest& req, HttpResponse* resp) override;
private:
    YiyiCatServer* server_;
};