cmake_minimum_required(VERSION 3.16)
project(FastUSRCompilerGUI)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
set(CMAKE_AUTOMOC ON)

add_executable(FastUSRCompilerGUI main.cpp)
target_link_libraries(FastUSRCompilerGUI Qt6::Widgets Qt6::Core Qt6::Gui)
