cmake_minimum_required(VERSION 3.16)
project(OpenGlShell)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

# ---- GLAD library ----
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ---- Your executable ----
add_executable(OpenGlShell
    src/main.cpp
)

target_include_directories(OpenGlShell PRIVATE external/include)

target_link_libraries(OpenGlShell PRIVATE
    glad
    glfw
    OpenGL::GL
    assimp::assimp
)

