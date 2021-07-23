cmake_minimum_required(VERSION 3.10)

project(PRT LANGUAGES CXX)

# find_package(CGAL CONFIG REQUIRED)
FIND_PACKAGE(embree 3.0 REQUIRED)
add_subdirectory("deps/imgui/")
add_subdirectory("deps/fmt-7.1.3/")
add_subdirectory("deps/sf_libs/")
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_PLY_IMPORTER TRUE)
set(ASSIMP_BUILD_PLY_EXPORTER TRUE)
add_subdirectory("deps/assimp/")

FILE ( GLOB PRT_SOURCES
    src/platform/*
    src/opengl/*
    src/util/*
    src/scene/*
    src/sh/*
    # src/cgal/*
    src/raytracing/light_probe.h
    src/raytracing/light_probe.cpp
    src/main.cpp
    ) 
add_executable(PRT ${PRT_SOURCES})

set_target_properties(PRT PROPERTIES
                      CXX_STANDARD 17
                      CXX_EXTENSIONS OFF)

target_compile_options(PRT PRIVATE /W4 /wd4201 /wd4840 /wd4100 /fp:fast /MP )


target_link_libraries(PRT 
    PRIVATE imgui fmt::fmt assimp sf_libs 
    # PRIVATE CGAL::CGAL
    PRIVATE embree
)

target_include_directories(PRT 
    PRIVATE "deps/" "deps/assimp/include"
    PRIVATE "src/"
    # PRIVATE "D:/jiahui/eigen-3.3.9" 
)

set_property(TARGET PRT PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
