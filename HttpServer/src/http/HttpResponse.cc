#include "HttpResponse.h"

void HttpResponse::setStatusLine(const std::string& version,
                                 HttpStatusCode statusCode,
                                 const std::string& statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

void HttpResponse::appendToBuffer(Buffer* outputBuf) const
{
    // 1. 状态行: "HTTP/1.1 200 OK\r\n"
    char versionBuf[32];
    snprintf(versionBuf, sizeof versionBuf, "%s %d ", httpVersion_.c_str(), static_cast<int>(statusCode_));
    
    outputBuf->append(versionBuf);
    outputBuf->append(statusMessage_.c_str());
    outputBuf->append("\r\n");

    // 2. 连接头处理
    if (closeConnection_) {
        outputBuf->append("Connection: close\r\n");
    } else {
        // Keep-Alive 连接必须包含 Content-Length
        char lengthBuf[64];
        snprintf(lengthBuf, sizeof lengthBuf, "Content-Length: %zd\r\n", body_.size());
        outputBuf->append(lengthBuf);
        outputBuf->append("Connection: Keep-Alive\r\n");
    }

    // 3. 添加服务器标识头（建议）
    outputBuf->append("Server: yiyiCatServer/1.0\r\n");

    // 4. 自定义头部
    for (const auto& [key, value] : headers_) {  // C++17 结构化绑定
        outputBuf->append(key.c_str());
        outputBuf->append(": "); 
        outputBuf->append(value.c_str());
        outputBuf->append("\r\n");
    }

    // 5. 头部与主体分隔空行
    outputBuf->append("\r\n");
    
    // 6. 响应体
    outputBuf->append(body_.c_str());
}