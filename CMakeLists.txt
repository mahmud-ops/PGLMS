cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)

# Include dirs
include_directories(
    include        # shader.h, shape.h
    include/glad   # glad.h
    include/GLFW   # if GLFW headers are local (optional)
    include/glm    # glm headers
)

# Source files
set(SOURCES
    src/main.cpp
    src/shader.cpp
    src/shape.cpp
    lib/glad.c
)

# Add executable
add_executable(OpenGLApp ${SOURCES})

# Link static or DLL version of GLFW (depending on what you have)
# Usually glfw3 for static, glfw3dll for dynamic
target_link_libraries(OpenGLApp glfw3 opengl32)
