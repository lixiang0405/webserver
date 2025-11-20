#pragma once

#include "SessionStorage.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <memory>
#include <random>
#include <functional>

class SessionManager
{
public:
    using GetSessionCallBack = std::function<void(const std::string&, const std::shared_ptr<Session>)>;
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    // 从请求中获取或创建会话
    std::shared_ptr<Session> getSession(const HttpRequest& req, HttpResponse* resp);

    std::shared_ptr<Session> getSession(std::string sessionId);

    // 销毁会话
    void destroySession(const std::string& sessionId);

    // 清理过期会话
    void cleanExpiredSessions();

    // 更新会话
    void updateSession(std::shared_ptr<Session> session)
    {
        storage_->save(session);
    }

    void setGetSessionCallBack(const GetSessionCallBack& getSessionCallBack) { getSessionCallBack_ = getSessionCallBack; }
private:
    std::string generateSessionId();
    std::string getSessionIdFromCookie(const HttpRequest& req);
    void setSessionCookie(const std::string& sessionId, HttpResponse* resp);
    
    std::unique_ptr<SessionStorage> storage_;
    std::mt19937 rng_; // 用于生成随机会话id
    GetSessionCallBack getSessionCallBack_;
};
