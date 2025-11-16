#pragma once
#include "RouterHandler.h"
#include "YiyiCatServer.h"

class EntryHandler : public RouterHandler 
{
public:
    explicit EntryHandler(YiyiCatServer* server) : server_(server) {}

    void handle(const HttpRequest& req,HttpResponse* resp) override;

private:
    YiyiCatServer* server_;
};