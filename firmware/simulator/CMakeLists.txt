cmake_minimum_required(VERSION 3.16)
project(revani_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LVGL, kartla aynı sürüm (firmware/platformio.ini)
include(FetchContent)
FetchContent_Declare(lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v9.2.2
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR _yok)  # LVGL'in kendi CMake'i kullanılmıyor, kaynaklar aşağıda derleniyor
FetchContent_MakeAvailable(lvgl)

find_package(SDL2 REQUIRED)

set(FW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

file(GLOB_RECURSE LVGL_SOURCES ${lvgl_SOURCE_DIR}/src/*.c)
add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC ${lvgl_SOURCE_DIR} ${FW_DIR}/config)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE REVANI_SIMULATOR)
target_link_libraries(lvgl PUBLIC SDL2::SDL2)

file(GLOB_RECURSE UI_SOURCES ${FW_DIR}/ui/*.c)
add_executable(revani_sim main.c screenshot.c bridge.c ${UI_SOURCES} ${FW_DIR}/protocol/protocol.c)
target_include_directories(revani_sim PRIVATE ${FW_DIR}/ui ${FW_DIR}/protocol)
target_compile_options(revani_sim PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(revani_sim PRIVATE lvgl m)

# Protokol birim testleri: ctest --test-dir build
enable_testing()
add_executable(protocol_test protocol_test.c ${FW_DIR}/protocol/protocol.c)
target_include_directories(protocol_test PRIVATE ${FW_DIR}/protocol)
target_compile_options(protocol_test PRIVATE -Wall -Wextra -Wno-unused-parameter)
add_test(NAME protocol COMMAND protocol_test)
