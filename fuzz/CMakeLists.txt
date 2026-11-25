# libFuzzer target for the monitor's input (aember-stats JSON lines): a host
# build with clang (ASan + UBSan) against the distribution's Qt 6, apart
# from the SDK build. fuzz/run.sh builds and runs it.
cmake_minimum_required(VERSION 3.25)
project(aember-monitor-fuzz LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "libFuzzer needs clang (CXX=clang++-18)")
endif()

set(SANITIZE -fsanitize=address,undefined -fno-sanitize-recover=all
             -fno-omit-frame-pointer)
add_compile_options(-g -O1 -Wall -Wextra ${SANITIZE} -fsanitize=fuzzer-no-link)
add_link_options(${SANITIZE})

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../src)
add_library(aember-monitor-lib STATIC ${SRC}/model.cpp ${SRC}/dashboard.cpp)
target_include_directories(aember-monitor-lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(aember-monitor-lib PUBLIC Qt6::Core Qt6::Gui)

add_executable(fuzz_sample fuzz_sample.cpp)
target_link_libraries(fuzz_sample PRIVATE aember-monitor-lib)
target_link_options(fuzz_sample PRIVATE -fsanitize=fuzzer)
target_compile_definitions(
  fuzz_sample PRIVATE AEMBER_MONITOR_FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../assets/fonts")
