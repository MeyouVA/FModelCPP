cmake_minimum_required(VERSION 3.20)
project(FModelCPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Defines both the CUE4Parse and CUE4Parse-Conversion targets — they are sibling projects inside one checkout,
# mirroring the C# repo layout.
add_subdirectory(CUE4Parse)

enable_testing()
add_subdirectory(tests)

# The FModel Qt app builds only when Qt6 is found (via CMAKE_PREFIX_PATH / Qt6_DIR). Without Qt the library and
# tests still configure and build normally.
find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    message(STATUS "Qt6 found (${Qt6_VERSION}) — building the FModel app.")
    add_subdirectory(FModel)
else()
    message(STATUS "Qt6 not found — skipping the FModel app (library + tests only). "
                   "Set -DCMAKE_PREFIX_PATH=<qt>/lib/cmake to enable it.")
endif()
