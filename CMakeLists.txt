cmake_minimum_required(VERSION 3.10)

project(PRT LANGUAGES CXX)

find_package(Eigen3 CONFIG REQUIRED)
add_subdirectory("deps/imgui/")
add_subdirectory("deps/nfd/")
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
    src/main.cpp
    ) 
add_executable(PRT ${PRT_SOURCES})

set_target_properties(PRT PROPERTIES
                      CXX_STANDARD 17
                      CXX_EXTENSIONS OFF)

target_compile_options(PRT PRIVATE /W4 /wd4201 /wd4840 /wd4100 /fp:fast /MP )


target_link_libraries(PRT 
    PRIVATE imgui nfd fmt::fmt assimp sf_libs  Eigen3::Eigen
)

target_include_directories(PRT 
    PRIVATE "deps/" "deps/assimp/include"
    PRIVATE "src/"
)
