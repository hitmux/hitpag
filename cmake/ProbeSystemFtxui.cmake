set(HITPAG_PROBE_FOUND FALSE)
set(HITPAG_PROBE_STATIC FALSE)
set(HITPAG_PROBE_REASON "compatible system FTXUI package not found")

if(DEFINED HITPAG_PROBE_PREFIX_PATH)
    string(REPLACE "|" ";" CMAKE_PREFIX_PATH "${HITPAG_PROBE_PREFIX_PATH}")
endif()

if(DEFINED HITPAG_PROBE_FIND_ROOT_PATH)
    string(REPLACE "|" ";" CMAKE_FIND_ROOT_PATH "${HITPAG_PROBE_FIND_ROOT_PATH}")
endif()

find_package(ftxui ${HITPAG_FTXUI_MIN_VERSION} QUIET)

if(ftxui_FOUND)
    set(HITPAG_PROBE_FOUND TRUE)
    set(HITPAG_PROBE_STATIC TRUE)
    set(HITPAG_PROBE_REASON "compatible system static FTXUI found")

    foreach(ftxui_target IN ITEMS ftxui::screen ftxui::dom ftxui::component)
        if(NOT TARGET ${ftxui_target})
            set(HITPAG_PROBE_STATIC FALSE)
            set(HITPAG_PROBE_REASON "system FTXUI package is incomplete")
            break()
        endif()

        get_target_property(ftxui_target_type ${ftxui_target} TYPE)
        if(ftxui_target_type STREQUAL "SHARED_LIBRARY")
            set(HITPAG_PROBE_STATIC FALSE)
            set(HITPAG_PROBE_REASON "system FTXUI is shared at runtime")
            break()
        endif()

        set(ftxui_locations)
        get_target_property(ftxui_imported_configs ${ftxui_target} IMPORTED_CONFIGURATIONS)
        if(ftxui_imported_configs)
            foreach(ftxui_config IN LISTS ftxui_imported_configs)
                get_target_property(ftxui_location ${ftxui_target} IMPORTED_LOCATION_${ftxui_config})
                if(ftxui_location)
                    list(APPEND ftxui_locations ${ftxui_location})
                endif()
            endforeach()
        endif()

        get_target_property(ftxui_default_location ${ftxui_target} IMPORTED_LOCATION)
        if(ftxui_default_location)
            list(APPEND ftxui_locations ${ftxui_default_location})
        endif()

        foreach(ftxui_location IN LISTS ftxui_locations)
            if(ftxui_location MATCHES "\\.so(\\.|$)|\\.dylib$|\\.dll$")
                set(HITPAG_PROBE_STATIC FALSE)
                set(HITPAG_PROBE_REASON "system FTXUI is shared at runtime")
                break()
            endif()
        endforeach()

        if(NOT HITPAG_PROBE_STATIC)
            break()
        endif()
    endforeach()
endif()

file(WRITE "${OUTPUT_FILE}"
    "FOUND=${HITPAG_PROBE_FOUND}\n"
    "STATIC=${HITPAG_PROBE_STATIC}\n"
    "REASON=${HITPAG_PROBE_REASON}\n")
