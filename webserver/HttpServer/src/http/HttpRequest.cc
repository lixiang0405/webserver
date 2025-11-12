#include "HttpRequest.h"
#include <string>
#include <sstream>

bool HttpRequest::setMethod(const char* start, const char* end){
    std::string method(start, end);
    if (method == "GET") {
        method_ = Method::GET;
    } else if (method == "POST") {
        method_ = Method::POST;
    } else if (method == "PUT") {
        method_ = Method::PUT;
    } else if (method == "DELETE") {
        method_ = Method::DELETE;
    } else if (method == "HEAD") {
        method_ = Method::HEAD;
    } else {
        method_ = Method::INVALID;
        return false;  // 无效方法
    }   
    return true;
}

void HttpRequest::setPath(const char* start, const char* end){
    path_ = std::string(start, end);
}

void HttpRequest::setVersion(std::string version){
    version_ = version;
}

void HttpRequest::setReceiveTime(TimeStamp t){
    receiveTime_ = t;
}

void HttpRequest::setQueryParameters(const char* start, const char* end) {
    std::string query_str(start, end);
    std::stringstream ss(query_str);
    std::string item;

    // 按 '&' 分割
    while (std::getline(ss, item, '&')) {
        // 每个 item 是 key=value 形式
        size_t pos = item.find('=');
        if (pos != std::string::npos) {
            std::string key = item.substr(0, pos);
            std::string value = item.substr(pos + 1);
            queryParameters_[key] = value;
        } else {
            // 如果没有等号，则视为只有key，值为空
            queryParameters_[item] = "";
        }
    }
}

void HttpRequest::addHeader(const char* start, const char* colon, const char* end){
    std::string key = std::string(start, colon);
    std::string value = std::string(colon + 1, end);
    headers_[key] = value;
}

void HttpRequest::setContentLength(size_t len){
    contentLength_ = len;
}

void HttpRequest::setBody(const std::string& body){
    content_ = body;
}

std::string HttpRequest::getHeader(const std::string &field) const{
    std::string result;
    auto it = headers_.find(field);
    if (it != headers_.end()){
        result = it->second;
    }
    return result;
}

std::string HttpRequest::getPathParameters(const std::string &key) const{
    std::string result;
    auto it = pathParameters_.find(key);
    if(it != pathParameters_.end()){
        result = it->second;
    } 
    return result;
}

std::string HttpRequest::getQueryParameters(const std::string &key) const{
        std::string result;
    auto it = queryParameters_.find(key);
    if(it != queryParameters_.end()){
        result = it->second;
    } 
    return result;
}

void HttpRequest::swap(HttpRequest& that){
    std::swap(method_, that.method_);
    std::swap(path_, that.path_);
    std::swap(pathParameters_, that.pathParameters_);
    std::swap(queryParameters_, that.queryParameters_);
    std::swap(version_, that.version_);
    std::swap(headers_, that.headers_);
    std::swap(receiveTime_, that.receiveTime_);
}