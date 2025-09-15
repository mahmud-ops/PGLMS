cmake_minimum_required(VERSION 3.10)
project(power_grid)

set(CMAKE_CXX_STANDARD 17)

# Include dirs
include_directories(
    include        # stb_image.h, imgui headers
    include/glad   # glad.h
    include/GLFW   # if GLFW headers are local (optional)
    include/glm    # glm headers
)

# Source files
set(SOURCES
    src/main.cpp
    src/glad.c
    src/imgui.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_impl_glfw.cpp
    src/imgui_impl_opengl3.cpp
    src/imgui_tables.cpp
    src/imgui_widgets.cpp
)

# Add executable
add_executable(power_grid ${SOURCES})

# Link static or DLL version of GLFW (depending on what you have)
# Usually glfw3 for static, glfw3dll for dynamic
target_link_libraries(power_grid glfw3 opengl32 winmm)
