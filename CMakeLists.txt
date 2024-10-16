cmake_minimum_required(VERSION 3.29)
project(NekoLang C)

# Set the C standard to C11
set(CMAKE_C_STANDARD 11)

# Find GTK+ 3 package
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Include the directories for GTK
include_directories(${GTK3_INCLUDE_DIRS})

# Link the necessary GTK libraries
link_directories(${GTK3_LIBRARY_DIRS})

# Suppress some warnings related to GTK libraries
add_definitions(${GTK3_CFLAGS_OTHER})

# Add the executable
add_executable(NekoLang neko.c)

# Link the GTK libraries with the executable
target_link_libraries(NekoLang ${GTK3_LIBRARIES})

