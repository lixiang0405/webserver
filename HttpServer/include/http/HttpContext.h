#pragma once

#include "Buffer.h"
#include "HttpRequest.h"


class HttpContext{
private:
    enum HttpRequestRarseState {
        kExpectRequestLine = 0,  // 等待解析请求行
        kExpectHeaders,      // 等待解析头部
        kExpectBody,         // 等待解析请求体
        kGotAll             // 解析完成
    };
    HttpRequestRarseState state_;
    HttpRequest request_;
public:
    HttpContext(/* args */): state_(kExpectRequestLine){}
    ~HttpContext();
    bool gotAll() const { return state_ == kGotAll;  }
    void reset(){
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
    }
    const HttpRequest& request() const { return request_;}
    HttpRequest& request() { return request_;}
    bool parseRequest(Buffer *buf, TimeStamp receiveTime);
    bool processRequestLine(const char *begin, const char *end);
    
};