if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

set(test_build_dir "/opt/hitpag/tmp/cmake_ftxui_vendored_test")
file(REMOVE_RECURSE "${test_build_dir}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${SOURCE_DIR}"
        -B "${test_build_dir}"
        -DHITPAG_PREFER_SYSTEM_FTXUI=OFF
        -DBUILD_SHARED_LIBS=ON
    RESULT_VARIABLE configure_status
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error
)

if(NOT configure_status EQUAL 0)
    file(REMOVE_RECURSE "${test_build_dir}")
    message(FATAL_ERROR "vendored FTXUI configure failed:\n${configure_output}\n${configure_error}")
endif()

if(NOT configure_output MATCHES "Using vendored static FTXUI")
    file(REMOVE_RECURSE "${test_build_dir}")
    message(FATAL_ERROR "configure did not select vendored FTXUI:\n${configure_output}\n${configure_error}")
endif()

file(READ "${test_build_dir}/CMakeCache.txt" cache_content)
if(NOT cache_content MATCHES "BUILD_SHARED_LIBS:UNINITIALIZED=ON")
    file(REMOVE_RECURSE "${test_build_dir}")
    message(FATAL_ERROR "BUILD_SHARED_LIBS cache value was not preserved as ON")
endif()

file(REMOVE_RECURSE "${test_build_dir}")
