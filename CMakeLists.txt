cmake_minimum_required(VERSION 3.10)
project(hitpag)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(hitpag
    src/main.cpp
    src/target_conflict.cpp
)

# 链接文件系统库
find_package(Threads REQUIRED)
target_link_libraries(hitpag PRIVATE stdc++fs Threads::Threads)

# 安装目标
install(TARGETS hitpag DESTINATION bin)
