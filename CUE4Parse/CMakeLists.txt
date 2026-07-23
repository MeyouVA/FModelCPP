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

# ---------------------------------------------------------------------------------------------------------
# CUE4Parse-Conversion — C++ port of the CUE4Parse-Conversion C# project (mesh/texture/animation exporters).
# A sibling project inside the same checkout, exactly as in the C# repo.
#
# Only the export-format enums are ported so far: they are what UserSettings persists and what the settings UI
# binds to, so they are needed by the FModel app long before the exporters themselves. Because nothing here has
# a translation unit yet, this is an INTERFACE (header-only) target; it becomes a STATIC library the moment the
# first .cpp lands, at which point the GLOB below just starts finding sources.
file(GLOB_RECURSE CUE4PARSE_CONVERSION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/CUE4Parse-Conversion/*.cpp")

if(CUE4PARSE_CONVERSION_SOURCES)
    add_library(CUE4Parse-Conversion STATIC ${CUE4PARSE_CONVERSION_SOURCES})
    target_include_directories(CUE4Parse-Conversion PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/CUE4Parse-Conversion")
    target_link_libraries(CUE4Parse-Conversion PUBLIC CUE4Parse)
    if(MSVC)
        target_compile_options(CUE4Parse-Conversion PRIVATE /W3 /permissive-)
    else()
        target_compile_options(CUE4Parse-Conversion PRIVATE -Wall -Wextra)
    endif()
else()
    add_library(CUE4Parse-Conversion INTERFACE)
    target_include_directories(CUE4Parse-Conversion INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/CUE4Parse-Conversion")
    target_link_libraries(CUE4Parse-Conversion INTERFACE CUE4Parse)
endif()
