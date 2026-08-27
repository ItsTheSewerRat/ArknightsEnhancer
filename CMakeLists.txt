cmake_minimum_required(VERSION 3.21)

project(ArknightsEnhancer VERSION 1.1.2 LANGUAGES CXX)

set(RESHADE_SDK_DIR "" CACHE PATH "Path to a ReShade 6.7.3 source tree")

if(NOT EXISTS "${RESHADE_SDK_DIR}/include/reshade.hpp")
    message(FATAL_ERROR
        "RESHADE_SDK_DIR must point to a ReShade 6.7.3 source tree. "
        "Run build.ps1 to download the matching SDK automatically.")
endif()

add_library(ArknightsEnhancer SHARED
    src/arknights_enhancer.cpp
    src/window_enhancements.cpp
)

target_compile_features(ArknightsEnhancer PRIVATE cxx_std_17)
target_compile_definitions(ArknightsEnhancer PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)
target_include_directories(ArknightsEnhancer SYSTEM PRIVATE
    "${RESHADE_SDK_DIR}/include"
)
target_link_libraries(ArknightsEnhancer PRIVATE
    dwmapi
    gdi32
    ole32
    user32
)

if(MSVC)
    target_compile_options(ArknightsEnhancer PRIVATE
        /W4
        /WX
        /permissive-
        /utf-8
        /guard:cf
    )
    target_link_options(ArknightsEnhancer PRIVATE /guard:cf)
endif()

set_target_properties(ArknightsEnhancer PROPERTIES
    OUTPUT_NAME "ArknightsEnhancer"
    PREFIX ""
    SUFFIX ".addon64"
)
