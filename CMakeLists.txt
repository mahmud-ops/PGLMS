cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)

# Include GLAD and GLFW
include_directories(include)
link_directories(lib)

# Add glad.c manually
add_library(glad STATIC lib/glad.c)

# Executable
add_executable(OpenGLApp src/main.cpp src/shader.cpp)

# Link GLFW and GLAD
target_link_libraries(OpenGLApp glad glfw3 opengl32)
