cmake_minimum_required(VERSION 3.29)
project(NekoLang C)

# Set the C standard to C11
set(CMAKE_C_STANDARD 11)

# Find OpenGL and GLFW packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# Include the directories for GLFW
include_directories(${GLFW_INCLUDE_DIRS})

# Add the executable
add_executable(NekoLang neko.c)

# Link the necessary OpenGL and GLFW libraries
target_link_libraries(NekoLang ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})