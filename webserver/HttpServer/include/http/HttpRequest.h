#pragma once

#include "TimeStamp.h"

#include <map>
#include <string>
#include <unordered_map>
#include <cstdint>


enum class Method{ GET = 0, POST, PUT, DELETE, HEAD, INVALID};

class HttpRequest{
private:
    Method                                       method_; // 请求方法
    std::string                                  version_; // http版本
    std::string                                  path_; // 请求路径
    std::unordered_map<std::string, std::string> pathParameters_; // 路径参数
    std::unordered_map<std::string, std::string> queryParameters_; // 查询参数
    TimeStamp                                    receiveTime_; // 接收时间
    std::map<std::string, std::string>           headers_; // 请求头
    std::string                                  content_; // 请求体
    uint64_t                                     contentLength_ { 0 }; // 请求体长度
public:
    HttpRequest()
        : method_(Method::INVALID)
        , version_("Unknown")
    {}

    ~HttpRequest();

    bool setMethod(const char* start, const char* end);

    void setReceiveTime(TimeStamp t);

    void setContentLength(uint64_t len);

    void setBody(const std::string& body);

    void setBody(const char* start, const char* end){ 
        if (end >= start){
            content_.assign(start, end - start); 
        }
    }

    void setPath(const char* start, const char* end);

    void setQueryParameters(const char* start, const char* end);

    void setVersion(std::string version);

    void addHeader(const char* start, const char* colon, const char* end);

    TimeStamp receiveTime() const { return receiveTime_; }

    Method method() const { return method_; }

    std::string path() const { return path_; }

    std::string getPathParameters(const std::string &key) const;

    std::string getQueryParameters(const std::string &key) const;

    std::string getVersion() const{ return version_; }
    
    std::string getHeader(const std::string &field) const;

    const std::map<std::string, std::string>& headers() const { return headers_; }

    uint64_t contentLength() const { return contentLength_; }
        
    void swap(HttpRequest& that);
};

