#include "HttpContext.h"
#include "Logger.h"

#include <algorithm>

bool HttpContext::parseRequest(Buffer *buf, TimeStamp receiveTime){
    bool ok = true;
    bool hasMore = true;
    std::string msg(buf->peek(), buf->readableBytes());
    LOG_INFO("HttpContext::parseRequest ? %.900s\n", msg.c_str());
    while (hasMore)
    {
        if (state_ == kExpectRequestLine) {
            LOG_DEBUG("解析请求行\n");
            const char *crlf = buf->findCRLF();  // 查找行结束符
            if (crlf) {
                // 处理请求行 (GET /index.html HTTP/1.1)
                ok = processRequestLine(buf->peek(), crlf);
                if (ok) {
                    request_.setReceiveTime(receiveTime);  // 设置接收时间
                    buf->retrieveUntil(crlf + 2);  // 移动读指针，跳过 CRLF
                    state_ = kExpectHeaders;  // 进入下一个状态
                } else {
                    LOG_DEBUG("解析失败，退出循环\n");
                    hasMore = false;  // 解析失败，退出循环
                }
            } else {
                LOG_DEBUG("数据不完整，等待更多数据\n");
                hasMore = false;  // 数据不完整，等待更多数据
            }
        }else if (state_ == kExpectHeaders) {
            LOG_DEBUG("解析请求头\n");
            const char *crlf = buf->findCRLF();
            if (crlf) {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon < crlf) {
                    // 解析头部字段 (Content-Type: text/html)
                    request_.addHeader(buf->peek(), colon, crlf);
                } else if (buf->peek() == crlf) {
                    // 空行，头部结束
                    if (!request_.getHeader("Content-Length").empty()) {
                        // 设置内容长度
                        request_.setContentLength(std::stoi(request_.getHeader("Content-Length")));
                        if (request_.contentLength() == 0) {
                            // 没有请求体，解析完成
                            LOG_DEBUG("没有请求体，解析完成\n");
                            state_ = kGotAll;
                            hasMore = false;
                        } else {
                            // 有请求体，进入下一个状态
                            state_ = kExpectBody;
                        }
                    } else {
                        // 没有 Content-Length，解析完成
                        LOG_DEBUG("没有Content-Length，解析完成\n");
                        state_ = kGotAll;
                        hasMore = false;
                    }
                } else {
                    // 头部格式错误
                    LOG_DEBUG("头部格式错误\n");
                    ok = false;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);  // 移动到下一行
            } else {
                LOG_DEBUG("数据不完整，等待更多数据\n");
                hasMore = false;  // 数据不完整
            }
        }else if (state_ == kExpectBody) {
            LOG_DEBUG("解析请求体\n");
            // 检查缓冲区中是否有足够的数据
            if (buf->readableBytes() < request_.contentLength()) {
                LOG_DEBUG("数据不完整，等待更多数据\n");
                hasMore = false;  // 数据不完整，等待更多数据
            } else {
                // 读取指定长度的请求体
                std::string body(buf->peek(), buf->peek() + request_.contentLength());
                request_.setBody(body);
                // 移动读指针
                buf->retrieve(request_.contentLength());
                state_ = kGotAll;  // 解析完成
                LOG_DEBUG("解析完成\n");
                hasMore = false;
            }
        }
    }
    return ok;
}

bool HttpContext::processRequestLine(const char *begin, const char *end) {
    bool succeed = false;
    const char* start = begin;
    
    // 1. 解析请求方法 (GET/POST/PUT等)
    const char* space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space)) {
        start = space + 1;  // 移动到路径开始位置
        
        // 2. 解析路径和查询参数
        space = std::find(start, end, ' ');
        if (space != end) {
            const char *argumentStart = std::find(start, space, '?');
            
            if (argumentStart != space) {  // 请求带参数
                // 设置路径 (问号之前的部分)
                request_.setPath(start, argumentStart);
                // 设置查询参数 (问号之后的部分)
                request_.setQueryParameters(argumentStart + 1, space);
            } else {  // 请求不带参数
                request_.setPath(start, space);
            }
            
            // 3. 解析 HTTP 版本
            start = space + 1;
            succeed = ((end - start == 8) && std::equal(start, end - 1, "HTTP/1."));
            
            if (succeed) {
                // 检查是 HTTP/1.1 还是 HTTP/1.0
                if (*(end - 1) == '1') {
                    request_.setVersion("HTTP/1.1");
                } else if (*(end - 1) == '0') {
                    request_.setVersion("HTTP/1.0");
                } else {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}