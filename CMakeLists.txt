cmake_minimum_required(VERSION 3.10)
project(hitpag)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hitpag
    src/main.cpp
    src/lib/util.cpp
    src/lib/i18n.cpp
    src/lib/error.cpp
    src/lib/progress.cpp
    src/lib/file_filter.cpp
    src/lib/file_type.cpp
    src/lib/args.cpp
    src/lib/operation.cpp
    src/lib/interactive.cpp
    src/lib/target_path.cpp
    src/lib/target_conflict.cpp
)

target_include_directories(hitpag PRIVATE src)

find_package(Threads REQUIRED)
target_link_libraries(hitpag PRIVATE Threads::Threads)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(hitpag PRIVATE stdc++fs)
endif()

install(TARGETS hitpag DESTINATION bin)
