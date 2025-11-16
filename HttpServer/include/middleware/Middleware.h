#pragma once

#include <memory>

#include "HttpRequest.h"
#include "HttpResponse.h"

class Middleware 
{
public:
    virtual ~Middleware() = default;
    
    // 请求前处理
    virtual void before(HttpRequest& request) = 0;
    
    // 响应后处理
    virtual void after(HttpResponse& response) = 0;
    
    // 设置下一个中间件
    void setNext(std::shared_ptr<Middleware> next) 
    {
        nextMiddleware_ = next;
    }

protected:
    std::shared_ptr<Middleware> nextMiddleware_;
};

