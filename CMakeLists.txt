cmake_minimum_required(VERSION 3.18)
project("1brc")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    add_compile_options(
        # debug
        "$<$<CONFIG:Debug>:/W4>"
        "$<$<CONFIG:Debug>:/Zi>"

        # rel w/ debug info (for profiling)
        "$<$<CONFIG:RelWithDebInfo>:/Zi>"
        "$<$<CONFIG:RelWithDebInfo>:/O2>"

        # release
        "$<$<CONFIG:Release>:/O2>"
    )

    add_link_options(
        # debug
        "$<$<CONFIG:Debug>:/DEBUG:FULL>"
        "$<$<CONFIG:Debug>:/INCREMENTAL>"

        # rel w/ debug info (for profiling)
        "$<$<CONFIG:RelWithDebInfo>:/DEBUG:FULL>"
        "$<$<CONFIG:RelWithDebInfo>:/PROFILE>"
        # already implied
        "$<$<CONFIG:RelWithDebInfo>:/DEBUG:FULL>"
        "$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>"

        # release
        "$<$<CONFIG:Release>:/INCREMENTAL:NO>"
    )
else()
    add_compile_options(
        "-msse4.2"
        # debug
        "$<$<CONFIG:Debug>:-Wall>"
        "$<$<CONFIG:Debug>:-Wextra>"
        "$<$<CONFIG:Debug>:-g>"
        "$<$<CONFIG:Debug>:-gcodeview>"

        # rel w/ debug info (for profiling)
        "$<$<CONFIG:RelWithDebInfo>:-O3>"
        "$<$<CONFIG:RelWithDebInfo>:-g>"
        "$<$<CONFIG:RelWithDebInfo>:-gcodeview>"

        # release
        "$<$<CONFIG:Release>:-O3>"
    )

    if (WIN32)
        # On Windows, clang's lld-link supports the same flags as link.exe
        add_link_options(
            # debug
            "$<$<CONFIG:Debug>:LINKER:/DEBUG:FULL>"
            "$<$<CONFIG:Debug>:LINKER:/INCREMENTAL>"

            # rel w/ debug info (for profiling)
            "$<$<CONFIG:RelWithDebInfo>:LINKER:/DEBUG:FULL>"
            "$<$<CONFIG:RelWithDebInfo>:LINKER:/PROFILE>"
            # already implied
            "$<$<CONFIG:RelWithDebInfo>:LINKER:/DEBUG:FULL>"
            "$<$<CONFIG:RelWithDebInfo>:LINKER:/INCREMENTAL:NO>"

            # release
            "$<$<CONFIG:Release>:LINKER:/INCREMENTAL:NO>"
        )
    else()
        add_link_options(
            # debug
            "$<$<CONFIG:Debug>:LINKER:--pdb=>"

            # rel w/ debug info (for profiling)
            "$<$<CONFIG:RelWithDebInfo>:LINKER:--pdb=>"
        )
    endif()
endif()

add_executable("calculate_average")

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Set 'calculate_average' as the startup project in VS
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "calculate_average")
    
    set_property(TARGET "calculate_average" PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:$<PROJECT_NAME>>")
endif()

add_subdirectory(src)
