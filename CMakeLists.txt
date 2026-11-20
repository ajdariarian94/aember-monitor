cmake_minimum_required(VERSION 3.25)
project(aember-monitor VERSION 0.1.0 LANGUAGES CXX)

# Cross builds with the aember-resources SDK (v0.1.13+: a minimal Qt 6 with
# Core, Gui, Network and the linuxfb/offscreen platforms; no OpenGL, no
# widgets). `aember build -a <arch>` passes the SDK's OEToolchainConfig.cmake.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build the unit tests (x64 SDK)" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_library(aember-monitor-lib STATIC src/model.cpp src/dashboard.cpp
                                      src/monitor-window.cpp)
target_include_directories(aember-monitor-lib PUBLIC include)
target_link_libraries(aember-monitor-lib PUBLIC Qt6::Core Qt6::Gui Qt6::Network)

add_executable(aember-monitor src/main.cpp)
target_link_libraries(aember-monitor PRIVATE aember-monitor-lib)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_link_options(aember-monitor PRIVATE -s)
endif()

install(TARGETS aember-monitor RUNTIME DESTINATION usr/bin)
install(FILES assets/fonts/DejaVuSans.ttf assets/fonts/LICENSE-DejaVu.txt
        DESTINATION usr/share/aember-monitor/fonts)

# Qt loads its platform plugin with dlopen, so no NEEDED entry pulls it in:
# install the two the app uses next to Qt's default plugin path
# (/usr/lib/plugins in the SDK's Qt). aember-build-helper then adds their
# libraries from the sysroot.
foreach(plugin Qt6::QLinuxFbIntegrationPlugin Qt6::QOffscreenIntegrationPlugin)
  if(NOT TARGET ${plugin})
    message(FATAL_ERROR "${plugin} missing in the SDK's Qt")
  endif()
  install(IMPORTED_RUNTIME_ARTIFACTS ${plugin}
          LIBRARY DESTINATION usr/lib/plugins/platforms)
endforeach()

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
