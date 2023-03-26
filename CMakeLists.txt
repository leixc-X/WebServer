cmake_minimum_required(VERSION 3.16)
project(LinuxWebServer)

set(CMAKE_CXX_STANDARD 11)  # 指定 C++ 版本

message("${PROJECT_SOURCE_DIR}=" ${PROJECT_SOURCE_DIR})

# 这里设置好路径后，进入子模块的cmake时不用再次设置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})  # 设置可执行文件的输出目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)	   # 设置库文件的输出目录

add_subdirectory(CGImysql)
add_subdirectory(http)
add_subdirectory(log)

# Header-only 的库可以添加为 INTERFACE 类型的 library
add_library(libthread INTERFACE)
target_include_directories(libthread INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/threadpool)

add_library(liblock INTERFACE)
target_include_directories(liblock INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lock)

add_library(libtimer INTERFACE)
target_include_directories(libtimer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/timer)

# 编译main，生成可执行文件
add_executable(server main.cpp)
target_link_libraries(server libSqlPool libHttp libLog libthread liblock libtimer pthread mysqlclient)  # 链接所有库
