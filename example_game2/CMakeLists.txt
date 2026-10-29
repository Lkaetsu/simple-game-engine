cmake_minimum_required(VERSION 3.14)
project(ExampleGame)

# 1. Download and include dependencies 
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.4
)

# Disable building GLFW's own examples and tests to speed up your build
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. Define your executable
set(SOURCES
    ../external_headers/glad.c
    game.cpp
)
add_executable(game ${SOURCES})

# 3. Define included directories
target_include_directories(game PRIVATE "../external_headers" "../headers")

# 4. Link Libraries
target_link_libraries(game PRIVATE glfw opengl32)