cmake_minimum_required(VERSION 3.14)
project(simpleGameEngine)

# 1. Download and include dependencies 
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.4
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.92.7
)
FetchContent_Declare(
    nativefiledialog-extended
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    GIT_TAG v1.3.0
)

# Disable building GLFW's own examples and tests to speed up your build
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(imgui)
FetchContent_MakeAvailable(nativefiledialog-extended)

# 2. Define your executable
set(SOURCES
    gui.cpp
    _deps/imgui-src/imgui_demo.cpp
    _deps/imgui-src/imgui_draw.cpp
    _deps/imgui-src/imgui_tables.cpp
    _deps/imgui-src/imgui_widgets.cpp
    _deps/imgui-src/imgui.cpp
    _deps/imgui-src/backends/imgui_impl_glfw.cpp
    _deps/imgui-src/backends/imgui_impl_opengl3.cpp
)
add_executable(SGE ${SOURCES})

# 3. Define included directories
target_include_directories(SGE PRIVATE "_deps/imgui-src" "external_headers")


# 4. Link Libraries
target_link_libraries(SGE PRIVATE glfw opengl32 nfd)