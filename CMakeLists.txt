cmake_minimum_required(VERSION 3.16)
project(fcitx5-enhint VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Utils REQUIRED)
find_package(Fcitx5Config REQUIRED)

# fcitx5 addons are loaded with dlopen, so build module (not link) libraries.
set(FCITX_ADDON_CMAKE_LIBRARY_TYPE MODULE)

add_subdirectory(src)
