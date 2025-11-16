#include "EntryHandler.h"
#include "LoginHandler.h"
#include "RegisterHandler.h"
#include "MenuHandler.h"
#include "AiGameStartHandler.h"
#include "LogoutHandler.h"
#include "AiGameMoveHandler.h"
#include "GameBackendHandler.h"
#include "YiyiCatServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpServer.h"

//有问题
YiyiCatServer::YiyiCatServer(int port,
                           const std::string &name,
                           TcpServer::Option option)
    : httpServer_(port, name, option), maxOnline_(0)
{
    initialize();
}

void YiyiCatServer::setThreadNum(int numThreads)
{
    httpServer_.setThreadNum(numThreads);
}

void YiyiCatServer::start()
{
    httpServer_.start();
}

void YiyiCatServer::initialize()
{
    // 初始化数据库连接池
    MysqlUtil::init("tcp://127.0.0.1", "admin", "030405", "yiyicat", 10);
    // 初始化会话
    initializeSession();
    // 初始化中间件
    initializeMiddleware();
    // 初始化路由
    initializeRouter();
}

void YiyiCatServer::initializeSession()
{
    // 创建会话存储
    auto sessionStorage = std::make_unique<MemorySessionStorage>();
    // 创建会话管理器
    auto sessionManager = std::make_unique<SessionManager>(std::move(sessionStorage));
    // 设置会话管理器
    setSessionManager(std::move(sessionManager));
}

void YiyiCatServer::initializeMiddleware()
{
    // 创建中间件
    auto corsMiddleware = std::make_shared<CorsMiddleware>();
    // 添加中间件
    httpServer_.addMiddleware(corsMiddleware);
}

void YiyiCatServer::initializeRouter()
{
    // 注册url回调处理器
    // 登录注册入口页面
    httpServer_.Get("/", std::make_shared<EntryHandler>(this));
    httpServer_.Get("/entry", std::make_shared<EntryHandler>(this));
    // 登录
    httpServer_.Post("/login", std::make_shared<LoginHandler>(this));
    // 注册
    httpServer_.Post("/register", std::make_shared<RegisterHandler>(this));
    // 登出
    httpServer_.Post("/user/logout", std::make_shared<LogoutHandler>(this));
    // 菜单页面
    httpServer_.Get("/menu", std::make_shared<MenuHandler>(this));
    // 开始对战ai
    httpServer_.Get("/aiBot/start", std::make_shared<AiGameStartHandler>(this));
    // 下棋
    httpServer_.Post("/aiBot/move", std::make_shared<AiGameMoveHandler>(this));
    // 重新开始对战ai
    httpServer_.Get("/aiBot/restart", 
    [this](const HttpRequest& req, HttpResponse* resp) {
            restartChessGameVsAi(req, resp);
    });

    // 后台界面
    httpServer_.Get("/backend", std::make_shared<GameBackendHandler>(this));
    // 后台数据获取
    httpServer_.Get("/backend_data", [this](const HttpRequest& req, HttpResponse* resp) {
        getBackendData(req, resp);
    });
}

void YiyiCatServer::restartChessGameVsAi(const HttpRequest &req, HttpResponse *resp)
{
    // 解析请求体
    auto session = getSessionManager()->getSession(req, resp);
    LOG_INFO("session->getValue(isLoggedIn) %s\n", session->getValue("isLoggedIn").c_str());
    if (session->getValue("isLoggedIn") != "true")
    {
        // 用户未登录，返回未授权错误
        json errorResp;
        errorResp["status"] = "error";
        errorResp["message"] = "Unauthorized";
        std::string errorBody = errorResp.dump(4);

        packageResp(req.getVersion(), HttpResponse::HttpStatusCode::k401Unauthorized,
                    "Unauthorized", true, "application/json", errorBody.size(),
                    errorBody, resp);
        return;
    }

    int userId = std::stoi(session->getValue("userId"));
    {
        // 重新开始ai对战
        std::lock_guard<std::mutex> lock(mutexForAiGames_);
        if (aiGames_.find(userId) != aiGames_.end())
            aiGames_.erase(userId);
        aiGames_[userId] = std::make_shared<AiGame>(userId);
    }

    json successResp;
    successResp["status"] = "ok";
    successResp["message"] = "restart successful";
    successResp["userId"] = userId;
    std::string successBody = successResp.dump(4);
    packageResp(req.getVersion(), HttpResponse::HttpStatusCode::k200Ok, "OK", false, "application/json", successBody.size(), successBody, resp);
}

// 获取后台数据
void YiyiCatServer::getBackendData(const HttpRequest &req, HttpResponse *resp)
{
    try 
    {
        // 获取数据
        int curOnline = getCurOnline();
        LOG_INFO("当前在线人数: %d\n", curOnline);
        
        int maxOnline = getMaxOnline();
        LOG_INFO("历史最高在线人数: %d\n", maxOnline);
        
        int totalUser = getUserCount();
        LOG_INFO("已注册用户总数: %d\n", totalUser);

        // 构造 JSON 响应
        nlohmann::json respBody;
        respBody = {
            {"curOnline", curOnline},
            {"maxOnline", maxOnline},
            {"totalUser", totalUser}
        };

        // 转换为字符串
        std::string responseStr = respBody.dump(4);
        
        // 设置响应
        resp->setStatusLine(req.getVersion(), HttpResponse::HttpStatusCode::k200Ok, "OK");
        resp->setContentType("application/json");
        resp->setBody(responseStr);
        resp->setContentLength(responseStr.size());
        resp->setCloseConnection(false);

        LOG_INFO("Backend data response prepared successfully\n");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error in getBackendData: %s\n", e.what());
        
        // 错误响应
        nlohmann::json errorBody = {
            {"error", "Internal Server Error"},
            {"message", e.what()}
        };
        
        std::string errorStr = errorBody.dump();
        resp->setStatusCode(HttpResponse::HttpStatusCode::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setContentType("application/json");
        resp->setBody(errorStr);
        resp->setContentLength(errorStr.size());
        resp->setCloseConnection(true);
    }
}

void YiyiCatServer::packageResp(const std::string &version,
                             HttpResponse::HttpStatusCode statusCode,
                             const std::string &statusMsg,
                             bool close,
                             const std::string &contentType,
                             int contentLen,
                             const std::string &body,
                             HttpResponse *resp)
{
    if (resp == nullptr) 
    {
        LOG_ERROR("Response pointer is null\n");
        return;
    }

    try 
    {
        resp->setVersion(version);
        resp->setStatusCode(statusCode);
        resp->setStatusMessage(statusMsg);
        resp->setCloseConnection(close);
        resp->setContentType(contentType);
        resp->setContentLength(contentLen);
        resp->setBody(body);
        
        LOG_INFO("Response packaged successfully\n");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error in packageResp: %s\n", e.what());
        // 设置一个基本的错误响应
        resp->setStatusCode(HttpResponse::HttpStatusCode::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setCloseConnection(true);
    }
}

