cmake_minimum_required(VERSION 3.10)
project(webserver)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 在文件开头添加 OpenSSL 查找
find_package(OpenSSL REQUIRED)

# 添加头文件路径
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/HttpServer/include/http
    ${PROJECT_SOURCE_DIR}/HttpServer/include/router
    ${PROJECT_SOURCE_DIR}/HttpServer/include/session
    ${PROJECT_SOURCE_DIR}/mymuduo/include
    ${PROJECT_SOURCE_DIR}/WebApps/yiyiCatServer/include
    /usr/include/mysql-cppconn-8
    /usr/include/mysql
    ${OPENSSL_INCLUDE_DIR}
)

# 查找必要的库
find_library(MYSQLCPPCONN_LIBRARY
    NAMES mysqlcppconn mysqlcppconn8
    PATHS /usr/lib /usr/lib64 /usr/local/lib
)

# 添加所有源文件
file(GLOB_RECURSE HTTP_SERVER_SRC
    "${PROJECT_SOURCE_DIR}/HttpServer/src/*.cc"
)

file(GLOB_RECURSE MYMUDUO_SRC
    "${PROJECT_SOURCE_DIR}/mymuduo/src/*.cc"
)


file(GLOB_RECURSE YIYICAT_SERVER_SRC
    "${PROJECT_SOURCE_DIR}/WebApps/yiyiCatServer/src/*.cc"
)

# 添加主程序文件
set(MAIN_SRC "${PROJECT_SOURCE_DIR}/WebApps/yiyiCatServer/src/main.cpp")

# 如果主程序文件不存在，创建一个
if(NOT EXISTS ${MAIN_SRC})
    file(WRITE ${MAIN_SRC}
"#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << \"no solution\" << '\n';
    return 0;
}
")
endif()

# 添加可执行文件
add_executable(webserver
    ${MAIN_SRC}
    ${MYMUDUO_SRC}
    ${HTTP_SERVER_SRC}
    ${YIYICAT_SERVER_SRC}
)

# 链接必要的库
target_link_libraries(webserver
    pthread
    # muduo_net
    # muduo_base
    mysqlcppconn
    mysqlclient
    ssl
    crypto
)

# 打印调试信息
message(STATUS "Include directories:")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    message(STATUS "  ${dir}")
endforeach()

message(STATUS "Main source file: ${MAIN_SRC}")
message(STATUS "mymuduo Server sources: ${MYMUDUO_SRC}")
message(STATUS "HTTP Server sources: ${HTTP_SERVER_SRC}")
message(STATUS "yiyiCat Server sources: ${YIYICAT_SERVER_SRC}")