cmake_minimum_required(VERSION 3.17)

project(glad LANGUAGES CXX)

set(SOURCES_GLAD "glad.cpp"
                 "glad/glad.h"
                 "glad/khrplatform.h"
                 )

add_library(glad STATIC ${SOURCES_GLAD})

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")
endif()

if(LINUX)
    target_link_libraries(glad ${CMAKE_DL_LIBS})
endif()

target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")