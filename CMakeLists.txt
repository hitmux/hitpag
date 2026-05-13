cmake_minimum_required(VERSION 3.14)
project(hitpag)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HITPAG_FTXUI_MIN_VERSION "5.0.0")
option(HITPAG_PREFER_SYSTEM_FTXUI "Prefer a compatible system static FTXUI package before falling back to the vendored static dependency" ON)
option(HITPAG_FORCE_VENDORED_FTXUI "Force using the vendored FTXUI dependency" OFF)

find_package(Threads REQUIRED)

include(cmake/HitpagFtxui.cmake)

if(HITPAG_USE_VENDORED_FTXUI)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ftxui/CMakeLists.txt")
        message(FATAL_ERROR
            "Compatible FTXUI (>= ${HITPAG_FTXUI_MIN_VERSION}, same major version) not found on the system, and vendored dependency is missing at third_party/ftxui")
    endif()

    set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(FTXUI_BUILD_TESTS_FUZZER OFF CACHE BOOL "" FORCE)
    set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    if(DEFINED BUILD_SHARED_LIBS)
        set(_hitpag_saved_build_shared_libs "${BUILD_SHARED_LIBS}")
    endif()
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(third_party/ftxui)
    if(DEFINED _hitpag_saved_build_shared_libs)
        set(BUILD_SHARED_LIBS "${_hitpag_saved_build_shared_libs}")
    else()
        unset(BUILD_SHARED_LIBS)
    endif()
    message(STATUS "Using vendored static FTXUI 5.0.0: ${HITPAG_FTXUI_DECISION_REASON}")
else()
    message(STATUS "Using system FTXUI compatible with >= ${HITPAG_FTXUI_MIN_VERSION}: ${HITPAG_FTXUI_DECISION_REASON}")
endif()

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
    src/lib/tui_main.cpp
    src/lib/tui_archive_ops.cpp
    src/lib/tui_archive_list.cpp
    src/lib/tui_details.cpp
    src/lib/tui_preview.cpp
    src/lib/tui_preview_model.cpp
    src/lib/tui_settings.cpp
    src/lib/tui_editor.cpp
)

enable_testing()

add_executable(tui_smoke_test
    tests/tui_smoke_test.cpp
    src/lib/args.cpp
    src/lib/error.cpp
    src/lib/i18n.cpp
    src/lib/file_type.cpp
    src/lib/operation.cpp
    src/lib/progress.cpp
    src/lib/tui_archive_ops.cpp
)

target_include_directories(hitpag PRIVATE src)
target_include_directories(tui_smoke_test PRIVATE src)

target_link_libraries(hitpag PRIVATE Threads::Threads
    ftxui::screen
    ftxui::dom
    ftxui::component
)

target_link_libraries(tui_smoke_test PRIVATE Threads::Threads)

add_test(NAME tui_smoke_test COMMAND tui_smoke_test)
add_test(
    NAME cmake_ftxui_vendored_test
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake_ftxui_vendored_test.cmake
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(hitpag PRIVATE stdc++fs)
    target_link_libraries(tui_smoke_test PRIVATE stdc++fs)
endif()

install(TARGETS hitpag DESTINATION bin)
