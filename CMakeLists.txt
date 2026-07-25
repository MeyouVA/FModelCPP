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

# ---------------------------------------------------------------------------------------------------
# Native codec libraries (Oodle, Zstd). They are runtime dependencies, not link-time ones: whatever the
# user drops in ThirdParty/native/<platform> is copied next to the built binaries so the search in
# CUE4Parse/Compression/NativeLibrary.cpp finds it without any absolute path being compiled in. An empty
# (or missing) folder is fine — the codecs simply stay unregistered. See ThirdParty/README.md.
# ---------------------------------------------------------------------------------------------------
if(WIN32)
    set(FMODELCPP_NATIVE_PLATFORM "win-x64")
    set(FMODELCPP_NATIVE_GLOB "*.dll")
else()
    set(FMODELCPP_NATIVE_PLATFORM "linux-x64")
    set(FMODELCPP_NATIVE_GLOB "*.so*")
endif()
set(FMODELCPP_NATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/native/${FMODELCPP_NATIVE_PLATFORM}")
file(GLOB FMODELCPP_NATIVE_LIBS "${FMODELCPP_NATIVE_DIR}/${FMODELCPP_NATIVE_GLOB}")
if(FMODELCPP_NATIVE_LIBS)
    message(STATUS "Native codec libraries from ${FMODELCPP_NATIVE_DIR}:")
    foreach(lib ${FMODELCPP_NATIVE_LIBS})
        get_filename_component(libname "${lib}" NAME)
        message(STATUS "  ${libname}")
        file(COPY "${lib}" DESTINATION "${CMAKE_BINARY_DIR}/tests")
        file(COPY "${lib}" DESTINATION "${CMAKE_BINARY_DIR}")
        if(Qt6_FOUND)
            file(COPY "${lib}" DESTINATION "${CMAKE_BINARY_DIR}/FModel")
        endif()
    endforeach()
else()
    message(STATUS "No native codec libraries in ${FMODELCPP_NATIVE_DIR} — Oodle/Zstd stay unregistered. "
                   "See ThirdParty/README.md.")
endif()
