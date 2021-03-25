cmake_minimum_required(VERSION 3.17)

project(imgui LANGUAGES CXX)

file(GLOB SOURCES_IMGUI ${PROJECT_SOURCE_DIR}/*.cpp)
add_library(imgui STATIC ${SOURCES_IMGUI})

add_subdirectory("glad/")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_INSTALL OFF CACHE BOOL " " FORCE)
set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
add_subdirectory("glfw/")
target_link_libraries(imgui 
    PUBLIC glfw glad
)
target_include_directories(imgui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    "glfw/include"
)

