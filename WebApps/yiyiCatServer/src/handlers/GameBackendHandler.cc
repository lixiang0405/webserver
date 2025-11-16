#include "GameBackendHandler.h"

void GameBackendHandler::handle(const HttpRequest& req, HttpResponse* resp)
{
    // 后台界面
    // 获取当前在线人数、历史最高在线人数、数据库中已注册用户总数
    std::string reqFile("../WebApps/yiyiCatServer/resource/Backend.html");
    FileUtil fileOperater(reqFile);
    if (!fileOperater.isValid())
    {
        LOG_ERROR("%s not exist\n", reqFile.c_str());
        fileOperater.resetDefaultFile();
    }

    std::vector<char> buffer(fileOperater.size());
    fileOperater.readFile(buffer); // 读出文件数据
    std::string htmlContent(buffer.data(), buffer.size());

    resp->setStatusLine(req.getVersion(), HttpResponse::HttpStatusCode::k200Ok, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("text/html");
    resp->setContentLength(htmlContent.size());
    resp->setBody(htmlContent);
}
