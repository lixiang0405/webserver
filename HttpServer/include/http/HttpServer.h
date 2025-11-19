#pragma once 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>

#include "TcpConnection.h"
#include "EventLoop.h"
#include "Logger.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Router.h"
#include "SessionManager.h"
#include "MiddlewareChain.h"
#include "CorsMiddleware.h"
#include "SslConnection.h"
#include "SslContext.h"
#include "TcpServer.h"

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable
{
public:
    using HttpCallback = std::function<void (const HttpRequest&, HttpResponse*)>;
    
    // 构造函数
    HttpServer(int port,
               const std::string& name,
               bool useSSL = false,
               TcpServer::Option option = TcpServer::kNoReusePort);
    
    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    void start(const std::string& certificateFile = "../WebApps/yiyiCatServer/resource/server.crt", const std::string& privateKeyFile = "../WebApps/yiyiCatServer/resource/server.key");

    EventLoop* getLoop() const 
    { 
        return server_.getLoop(); 
    }

    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }

    // 注册静态路由处理器
    void Get(const std::string& path, const HttpCallback& cb)
    {
        router_.registerCallback(HttpRequest::Method::kGet, path, cb);
    }
    
    // 注册静态路由处理器
    void Get(const std::string& path, Router::HandlerPtr handler)
    {
        router_.registerHandler(HttpRequest::Method::kGet, path, handler);
    }

    void Post(const std::string& path, const HttpCallback& cb)
    {
        router_.registerCallback(HttpRequest::Method::kPost, path, cb);
    }

    void Post(const std::string& path, Router::HandlerPtr handler)
    {
        router_.registerHandler(HttpRequest::Method::kPost, path, handler);
    }

    // 注册动态路由处理器
    void addRoute(HttpRequest::Method method, const std::string& path, Router::HandlerPtr handler)
    {
        router_.addRegexHandler(method, path, handler);
    }

    // 注册动态路由处理函数
    void addRoute(HttpRequest::Method method, const std::string& path, const Router::HandlerCallback& callback)
    {
        router_.addRegexCallback(method, path, callback);
    }

    // 设置会话管理器
    void setSessionManager(std::unique_ptr<SessionManager> manager)
    {
        sessionManager_ = std::move(manager);
    }

    // 获取会话管理器
    SessionManager* getSessionManager() const
    {
        return sessionManager_.get();
    }

    // 添加中间件的方法
    void addMiddleware(std::shared_ptr<Middleware> middleware) 
    {
        middlewareChain_.addMiddleware(middleware);
    }

    void enableSSL(bool enable) 
    {
        useSSL_ = enable;
    }

    void setSslConfig(const SslConfig& config);

private:
    void initialize();

    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   TimeStamp receiveTime);
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);

    void handleRequest(const HttpRequest& req, HttpResponse* resp);

    void send(const TcpConnectionPtr &conn, std::string msg);
    
private:
    InetAddress                                 listenAddr_; // 监听地址
    TcpServer                                   server_; 
    EventLoop                                   mainLoop_; // 主循环
    HttpCallback                                httpCallback_; // 回调函数
    Router                                      router_; // 路由
    std::unique_ptr<SessionManager>             sessionManager_; // 会话管理器
    MiddlewareChain                             middlewareChain_; // 中间件链
    std::unique_ptr<SslContext>                 sslCtx_; // SSL 上下文
    bool                                        useSSL_; // 是否使用 SSL   
    // TcpConnectionPtr -> SslConnectionPtr 
    std::map<TcpConnectionPtr, std::unique_ptr<SslConnection>> sslConns_;
}; 