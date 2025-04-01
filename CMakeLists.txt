cmake_minimum_required(VERSION 3.10)
project(mymuduo)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置动态库输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# 添加编译器选项
add_compile_options(-g)

# 显式列出源文件
set(SRC_LIST
    src/Logger.cc
    src/Timestamp.cc
    src/InetAddress.cc
    src/Channel.cc
    src/CurrentThread.cc
    src/Poller.cc
    src/DefaultPoller.cc
    src/EpollPoller.cc
    src/EventLoop.cc
    src/Thread.cc
    src/EventLoopThread.cc
    src/EventLoopThreadPool.cc
    src/Socket.cc
    src/Acceptor.cc
    src/Buffer.cc
    src/TcpConnection.cc
    src/TcpServer.cc
)

# 创建共享库
add_library(mymuduo SHARED ${SRC_LIST})

# 添加头文件路径
target_include_directories(mymuduo PUBLIC ${PROJECT_SOURCE_DIR}/include)

# 安装规则
# install(TARGETS mymuduo
#         LIBRARY DESTINATION lib
#         ARCHIVE DESTINATION lib
#         RUNTIME DESTINATION bin)

# install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)