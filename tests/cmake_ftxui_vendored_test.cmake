if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

# The scratch build tree has to live somewhere writable. Callers pass TEST_WORK_DIR (ctest
# supplies the project's binary directory) and the current directory is the fallback, because a
# hardcoded absolute path only ever works on the machine the suite was written on.
if(NOT DEFINED TEST_WORK_DIR)
    set(TEST_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

set(test_build_dir "${TEST_WORK_DIR}/cmake_ftxui_vendored_test")
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
