include(CMakeParseArguments)

option(HITPAG_FORCE_SYSTEM_FTXUI "Force using a compatible system FTXUI package even if it is shared at runtime" OFF)

function(hitpag_use_vendored_ftxui reason)
    set(HITPAG_USE_VENDORED_FTXUI ON PARENT_SCOPE)
    set(HITPAG_FTXUI_DECISION_REASON "${reason}" PARENT_SCOPE)
endfunction()

function(hitpag_use_system_ftxui reason)
    set(HITPAG_USE_VENDORED_FTXUI OFF PARENT_SCOPE)
    set(HITPAG_FTXUI_DECISION_REASON "${reason}" PARENT_SCOPE)
endfunction()

function(hitpag_probe_system_ftxui out_found_var out_static_var out_reason_var)
    set(probe_build_dir "${CMAKE_CURRENT_BINARY_DIR}/hitpag-ftxui-probe-build")
    set(probe_file "${probe_build_dir}/hitpag-ftxui-probe.txt")

    set(probe_prefix_path "")
    if(CMAKE_PREFIX_PATH)
        string(REPLACE ";" "|" probe_prefix_path "${CMAKE_PREFIX_PATH}")
    endif()

    set(probe_find_root_path "")
    if(CMAKE_FIND_ROOT_PATH)
        string(REPLACE ";" "|" probe_find_root_path "${CMAKE_FIND_ROOT_PATH}")
    endif()

    set(probe_cmake_args
        -S ${CMAKE_CURRENT_LIST_DIR}/ProbeSystemFtxuiProject
        -B ${probe_build_dir}
        -DOUTPUT_FILE=${probe_file}
        -DHITPAG_FTXUI_MIN_VERSION=${HITPAG_FTXUI_MIN_VERSION}
        -DHITPAG_PROBE_PREFIX_PATH=${probe_prefix_path}
        -DHITPAG_PROBE_FIND_ROOT_PATH=${probe_find_root_path}
    )

    if(CMAKE_GENERATOR)
        list(APPEND probe_cmake_args -G "${CMAKE_GENERATOR}")
    endif()
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND probe_cmake_args -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
        list(APPEND probe_cmake_args -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    if(CMAKE_MAKE_PROGRAM)
        list(APPEND probe_cmake_args -D CMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
    endif()
    if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND probe_cmake_args -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()
    if(CMAKE_SYSROOT)
        list(APPEND probe_cmake_args -D CMAKE_SYSROOT=${CMAKE_SYSROOT})
    endif()
    if(CMAKE_SYSTEM_NAME)
        list(APPEND probe_cmake_args -D CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})
    endif()
    if(CMAKE_SYSTEM_PROCESSOR)
        list(APPEND probe_cmake_args -D CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR})
    endif()
    if(CMAKE_C_COMPILER_LAUNCHER)
        list(APPEND probe_cmake_args -D CMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER})
    endif()
    if(CMAKE_CXX_COMPILER_LAUNCHER)
        list(APPEND probe_cmake_args -D CMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER})
    endif()

    set(probe_env)
    if(DEFINED ENV{CC})
        list(APPEND probe_env "CC=")
    endif()
    if(DEFINED ENV{CXX})
        list(APPEND probe_env "CXX=")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env ${probe_env} ${CMAKE_COMMAND} ${probe_cmake_args}
        RESULT_VARIABLE probe_status
    )

    if(NOT probe_status EQUAL 0 OR NOT EXISTS "${probe_file}")
        set(${out_found_var} FALSE PARENT_SCOPE)
        set(${out_static_var} FALSE PARENT_SCOPE)
        set(${out_reason_var} "failed to probe system FTXUI" PARENT_SCOPE)
        return()
    endif()

    file(STRINGS "${probe_file}" probe_lines)
    set(probe_found FALSE)
    set(probe_static FALSE)
    set(probe_reason "failed to probe system FTXUI")

    foreach(probe_line IN LISTS probe_lines)
        if(probe_line STREQUAL "FOUND=TRUE")
            set(probe_found TRUE)
        elseif(probe_line STREQUAL "STATIC=TRUE")
            set(probe_static TRUE)
        elseif(probe_line MATCHES "^REASON=(.*)$")
            set(probe_reason "${CMAKE_MATCH_1}")
        endif()
    endforeach()

    set(${out_found_var} ${probe_found} PARENT_SCOPE)
    set(${out_static_var} ${probe_static} PARENT_SCOPE)
    set(${out_reason_var} "${probe_reason}" PARENT_SCOPE)
endfunction()

set(HITPAG_USE_VENDORED_FTXUI ON)
set(HITPAG_FTXUI_DECISION_REASON "vendored fallback selected by default")

if(HITPAG_FORCE_VENDORED_FTXUI)
    hitpag_use_vendored_ftxui("forced by HITPAG_FORCE_VENDORED_FTXUI")
elseif(HITPAG_FORCE_SYSTEM_FTXUI)
    find_package(ftxui ${HITPAG_FTXUI_MIN_VERSION} REQUIRED)
    hitpag_use_system_ftxui("forced by HITPAG_FORCE_SYSTEM_FTXUI")
elseif(HITPAG_PREFER_SYSTEM_FTXUI)
    hitpag_probe_system_ftxui(HITPAG_SYSTEM_FTXUI_FOUND HITPAG_SYSTEM_FTXUI_IS_STATIC HITPAG_SYSTEM_FTXUI_REASON)
    if(HITPAG_SYSTEM_FTXUI_FOUND AND HITPAG_SYSTEM_FTXUI_IS_STATIC)
        find_package(ftxui ${HITPAG_FTXUI_MIN_VERSION} REQUIRED)
        hitpag_use_system_ftxui("${HITPAG_SYSTEM_FTXUI_REASON}")
    else()
        hitpag_use_vendored_ftxui("${HITPAG_SYSTEM_FTXUI_REASON}")
    endif()
endif()
