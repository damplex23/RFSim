cmake_minimum_required(VERSION 3.20)
project(RFSim VERSION 1.0.0 LANGUAGES CXX)

# Force C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Dependencies via FetchContent ---
include(FetchContent)

# 1. Raylib - Core Graphics and Audio
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# 2. Dear ImGui - Docking Branch for UI
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# 3. rlImGui - Raylib backend for ImGui
FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG        Raylib_5_5
)
FetchContent_GetProperties(rlImGui)
if(NOT rlimgui_POPULATED)
    FetchContent_Populate(rlImGui)
endif()

# --- System Dependencies ---
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# --- Project Source Configuration ---
set(PROJECT_SOURCES
    src/main.cpp
    src/UIManager.cpp
    src/MapEngine.cpp
    src/PropagationEngine.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)

set(PROJECT_HEADERS
    include/DataStructures.hpp
    include/UIManager.hpp
    include/MapEngine.hpp
    include/PropagationEngine.hpp
    ${rlimgui_SOURCE_DIR}/rlImGui.h
)

# --- Include ImGui Sources ---
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})

# --- Main Executable ---
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Include Paths
target_include_directories(${PROJECT_NAME} PRIVATE 
    include
    ${raylib_SOURCE_DIR}/src
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
)

# Linking
# Note: rlImGui source is included directly in PROJECT_SOURCES
target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    imgui_lib
    ${CURL_LIBRARIES}
    Threads::Threads
)

# Platform Specific Linking
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE m dl)
endif()

# Development Flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

message(STATUS "RFSim Project Configuration Complete.")
message(STATUS "Target: C++20")
message(STATUS "Libraries: Raylib, ImGui (Docking), rlImGui, libcurl")
