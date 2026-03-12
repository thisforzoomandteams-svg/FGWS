cmake_minimum_required(VERSION 3.20)

project(ImportedFrameGenFiles NONE)

file(GLOB IMPORTED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

file(GLOB IMPORTED_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(framegen_imports STATIC ${IMPORTED_SOURCES} ${IMPORTED_HEADERS})
target_include_directories(framegen_imports PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "Imported: ${IMPORTED_SOURCES}")
