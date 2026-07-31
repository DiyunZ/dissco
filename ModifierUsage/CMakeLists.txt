cmake_minimum_required(VERSION 3.25)

project(ModifierUsage LANGUAGES CXX)

add_library(ModifierUsage STATIC
    src/ModifierUsage.cpp
)

add_library(DISSCO::ModifierUsage ALIAS ModifierUsage)

target_include_directories(ModifierUsage
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(ModifierUsage PUBLIC cxx_std_20)
