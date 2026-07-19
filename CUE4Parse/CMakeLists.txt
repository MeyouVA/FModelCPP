# CUE4Parse — C++ port of the CUE4Parse C# library (asset/package parsing core).
# Built incrementally; currently covers the version and reader foundation.

file(GLOB_RECURSE CUE4PARSE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/CUE4Parse/*.cpp")

add_library(CUE4Parse STATIC ${CUE4PARSE_SOURCES})

target_include_directories(CUE4Parse PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/CUE4Parse")

if(MSVC)
    target_compile_options(CUE4Parse PRIVATE /W3 /permissive-)
else()
    target_compile_options(CUE4Parse PRIVATE -Wall -Wextra)
endif()
