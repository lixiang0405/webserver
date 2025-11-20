#pragma once

#include <atomic>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <mutex>


#include "AiGame.h"
#include "HttpServer.h"
#include "MysqlUtil.h"
#include "FileUtil.h"
#include "JsonUtil.h"


class LoginHandler;
class EntryHandler;
class RegisterHandler;
class MenuHandler;
class AiGameStartHandler;
class LogoutHandler;
class AiGameMoveHandler;
class GameBackendHandler;

#define DURING_GAME 1 
#define GAME_OVER 2

#define MAX_AIBOT_NUM 4096

class YiyiCatServer
{
public:
    YiyiCatServer(int port,
                  const std::string& name,
                  bool useSSL = false,
                  TcpServer::Option option = TcpServer::kNoReusePort);

    void setThreadNum(int numThreads);
    void start();
private:
    void initialize();
    void initializeSession();
    void initializeRouter();
    void initializeMiddleware();
    
    void setSessionManager(std::unique_ptr<SessionManager> manager)
    {
        httpServer_.setSessionManager(std::move(manager));
    }

    SessionManager*  getSessionManager() const
    {
        return httpServer_.getSessionManager();
    }
    
    void restartChessGameVsAi(const HttpRequest& req, HttpResponse* resp);
    void getBackendData(const HttpRequest& req, HttpResponse* resp);

    void packageResp(const std::string& version, HttpResponse::HttpStatusCode statusCode,
                     const std::string& statusMsg, bool close, const std::string& contentType,
                     int contentLen, const std::string& body, HttpResponse* resp);

    // 获取历史最高在线人数
    int getMaxOnline() const
    {
        return maxOnline_.load();
    }

    // 获取当前在线人数
    int getCurOnline() const
    {
        return onlineUsers_.size();
    }

    void updateMaxOnline(int online)
    {
        maxOnline_ = std::max(maxOnline_.load(), online);
    }

    // 获取用户总数
    int getUserCount()
    {
        std::string sql = "SELECT COUNT(*) as count FROM users";

        sql::ResultSet* res = mysqlUtil_.executeQuery(sql);
        if (res->next())
        {
            return res->getInt("count");
        }
        return 0;
    }

    void setUserIdSessionId(int userId, std::string sessionId){
        LOG_INFO("set userId:%d SessionId%s\n", userId, sessionId.c_str());
        userIdSessionId_[userId] = sessionId;
    }
    
private:
    friend class EntryHandler;
    friend class LoginHandler;
    friend class RegisterHandler;
    friend class MenuHandler;
    friend class AiGameStartHandler;
    friend class LogoutHandler;
    friend class AiGameMoveHandler;
    friend class GameBackendHandler;

private:
    enum GameType
    {
        NO_GAME = 0,
        MAN_VS_AI = 1,
        MAN_VS_MAN = 2
    };
    // 实际业务制定由yiyiCatServer来完成
    // 需要留意httpServer_提供哪些接口供使用
    HttpServer                                       httpServer_;
    MysqlUtil                                        mysqlUtil_;
    // userId -> AiBot
    std::unordered_map<int, std::shared_ptr<AiGame>> aiGames_;
    std::mutex                                       mutexForAiGames_;
    // userId -> 是否在游戏中
    std::unordered_map<int, bool>                    onlineUsers_;
    std::mutex                                       mutexForOnlineUsers_; 
    // 最高在线人数
    std::atomic<int>                                 maxOnline_;
    std::unordered_map<int, std::string>             userIdSessionId_;
                              
};