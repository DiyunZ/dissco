cmake_minimum_required(VERSION 3.25)

project(CMOD)
message(STATUS "== CMOD ==")

find_package(SndFile REQUIRED)

file(GLOB_RECURSE CMOD_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
)

list(REMOVE_ITEM CMOD_SRC
    "${PROJECT_SOURCE_DIR}/src/ModifierUsage.cpp"
    "${PROJECT_SOURCE_DIR}/src/ModifierUsage.hpp"
)

add_library(ModifierUsage STATIC
    src/ModifierUsage.cpp
    src/ModifierUsage.hpp
)
add_library(DISSCO::ModifierUsage ALIAS ModifierUsage)

target_include_directories(ModifierUsage PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
)
target_compile_features(ModifierUsage PUBLIC cxx_std_20)

add_executable(CMOD ${CMOD_SRC})
set(CMOD_EXECUTABLE_BASENAME "cmod")
set_target_properties(CMOD PROPERTIES
    OUTPUT_NAME "${CMOD_EXECUTABLE_BASENAME}"
)
# CMOD is consumed by LASSIE as a standalone subprocess (QProcess::start),
# not linked into it. No ENABLE_EXPORTS / WINDOWS_EXPORT_ALL_SYMBOLS is
# needed — and adding them just to "make the link work" backfires on
# Windows, where it requires an import library that no consumer actually uses.

### libsndfile must be linked in just the same way as it is in LASS ###
target_link_libraries(CMOD PUBLIC ${SNDFILE_LIBRARY})
target_link_libraries(CMOD PUBLIC PUGIXML)
# add_subdirectory(${CMAKE_SOURCE_DIR}/external-libs/muParser)
target_link_libraries(CMOD PUBLIC MUPARSER)
target_link_libraries(CMOD PUBLIC LASS)
target_link_libraries(CMOD PUBLIC DISSCO::ModifierUsage)
message(STATUS "libsndfile, pugixml, muParser, and LASS includes linked with target!")

# heavy libraries that won't change much from compile to compile in this project
target_precompile_headers(CMOD PRIVATE
    <pugixml.hpp>
    <sndfile.h>
    <vector>
    <string>
    <map>
    <memory>
    <iostream>
    <fstream>
    <sstream>
)

### _external_ library checkers for informative errors ###
if(NOT SNDFILE_FOUND)
    message(SEND_ERROR "Couldn't find libsndfile, won't go well; check BUILDING docs")
else()
    message(STATUS "Found libsndfile install!")
endif()

# set "default" location of the CMOD binary (directory) for use in LASSIE
if(NOT CMOD_BINARY)
    set(CMOD_BINARY
        "${CMAKE_CURRENT_BINARY_DIR}/${CMOD_EXECUTABLE_BASENAME}"
        PARENT_SCOPE
    )
    message(STATUS "CMOD_BINARY set")
endif()
message(STATUS "CMOD_BINARY set to ${CMOD_BINARY}")

include(GNUInstallDirs)
if(APPLE)
    install(TARGETS CMOD
        RUNTIME DESTINATION lassie.app/Contents/MacOS
        COMPONENT Runtime
    )
else()
    install(TARGETS CMOD
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
endif()

# Ship libsndfile's runtime DLL alongside the executables on Windows when
# FindSndFile resolved to the FetchContent prebuilt (vcpkg / brew / apt
# installs put it in a system-discoverable place and don't need this).
# windeployqt only knows about Qt DLLs, so this is on us.
if(WIN32 AND SNDFILE_DLL)
    install(FILES "${SNDFILE_DLL}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
endif()

if(BUILD_TESTING)
    add_executable(CMOD_ErrorReportingFormatTest tests/ErrorReportingFormatTest.cpp)
    target_include_directories(CMOD_ErrorReportingFormatTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    add_test(NAME CMOD.ErrorReporting.Format COMMAND CMOD_ErrorReportingFormatTest)

    foreach(error_case IN ITEMS
            no_arguments missing_project malformed_xml invalid_root missing_configuration
            missing_field invalid_integer zero_threads invalid_boolean
            children_without_layer negative_children negative_fractional_children
            negative_duration empty_score
            output_directory valid valid_synthesis)
        add_test(NAME CMOD.ErrorReporting.${error_case}
            COMMAND ${CMAKE_COMMAND}
                -DCMOD_EXECUTABLE=$<TARGET_FILE:CMOD>
                -DPROJECT_FIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ErrorReporting.dissco
                -DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/error-reporting/${error_case}
                -DTEST_CASE=${error_case}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ErrorReportingTest.cmake
        )
        set_tests_properties(CMOD.ErrorReporting.${error_case} PROPERTIES TIMEOUT 20)
    endforeach()

    foreach(error_case IN ITEMS
            project_expression event_expression nested_expression missing_top
            missing_object unknown_function select_negative select_object
            null_context null_static_context nonfinite_expression
            missing_markov_library invalid_markov_count temporary_library_write)
        add_test(NAME CMOD.ErrorReporting.${error_case}
            COMMAND ${CMAKE_COMMAND}
                -DCMOD_EXECUTABLE=$<TARGET_FILE:CMOD>
                -DPROJECT_FIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ErrorReporting.dissco
                -DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/function-errors
                -DTEST_CASE=${error_case}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/FunctionErrorTest.cmake
        )
        set_tests_properties(CMOD.ErrorReporting.${error_case} PROPERTIES TIMEOUT 20)
    endforeach()

    set(score_cases lilypond score_file)
    find_program(LILYPOND_EXECUTABLE lilypond)
    if(LILYPOND_EXECUTABLE)
        list(APPEND score_cases valid_score)
    endif()
    foreach(error_case IN LISTS score_cases)
        add_test(NAME CMOD.ErrorReporting.${error_case}
            COMMAND ${CMAKE_COMMAND}
                -DCMOD_EXECUTABLE=$<TARGET_FILE:CMOD>
                -DPROJECT_FIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ScoreErrorReporting.dissco
                -DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/error-reporting/${error_case}
                -DTEST_CASE=${error_case}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ErrorReportingTest.cmake
        )
        set_tests_properties(CMOD.ErrorReporting.${error_case} PROPERTIES TIMEOUT 20)
    endforeach()
endif()

message(STATUS "DONE!")
