#include <string>
#include <iostream>
#include "TcpServer.h"
#include "Logger.h"
#include "EventLoop.h"

#include "YiyiCatServer.h"

int main(int argc, char* argv[])
{
  LOG_INFO("pid = %d", static_cast<int>(getpid()));
  
  std::string serverName = "HttpServer";
  int port = 8080;
  bool useSSL = false;
  
  // 参数解析
  int opt;
  const char* str = "p:s";
  while ((opt = getopt(argc, argv, str)) != -1)
  {
    switch (opt)
    {
      case 'p':
      {
        port = atoi(optarg);
        break;
      }
      case 's':
      {
        useSSL = true;
        break;
      }
      default:
        break;
    }
  }
  
  // muduo::Logger::setLogLevel(muduo::Logger::WARN);
  YiyiCatServer server(port, serverName, useSSL);
  server.setThreadNum(4);
  server.start();
}
