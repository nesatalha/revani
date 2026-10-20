cmake_minimum_required(VERSION 3.16)
project(revani_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LVGL, kartla aynı sürüm (firmware/platformio.ini)
include(FetchContent)
FetchContent_Declare(lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v9.2.2
  GIT_SHALLOW TRUE)
FetchContent_GetProperties(lvgl)
if(NOT lvgl_POPULATED)
  FetchContent_Populate(lvgl)
endif()

find_package(SDL2 REQUIRED)

set(FW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

file(GLOB_RECURSE LVGL_SOURCES ${lvgl_SOURCE_DIR}/src/*.c)
add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC ${lvgl_SOURCE_DIR} ${FW_DIR}/config)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE REVANI_SIMULATOR)
target_link_libraries(lvgl PUBLIC SDL2::SDL2)

file(GLOB_RECURSE UI_SOURCES ${FW_DIR}/ui/*.c)
add_executable(revani_sim main.c screenshot.c ${UI_SOURCES})
target_include_directories(revani_sim PRIVATE ${FW_DIR}/ui)
target_compile_options(revani_sim PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(revani_sim PRIVATE lvgl m)
