// Ported (structurally) from CUE4Parse/Compression/OodleHelper.cs.
//
// Oodle is proprietary (RAD Game Tools) and cannot be vendored, so — exactly like the C# side, which loads a
// native oo2core / oodle-data-shared library at runtime — this helper dynamically loads the Oodle shared
// library and registers its OodleLZ_Decompress with the Compression registry. The C#'s HTTP auto-download of
// the DLL is not ported; point Initialize at an already-present library.
#pragma once

#include <string>

namespace CUE4Parse::Compression
{
    class OodleHelper
    {
    public:
        // Default library names tried when Initialize is called with an empty path.
        static constexpr const char* OODLE_NAME_OLD = "oo2core_9_win64.dll";
        static constexpr const char* OODLE_NAME_CURRENT = "oodle-data-shared.dll";
        static constexpr const char* OODLE_NAME_LINUX = "liboodle-data-shared.so";

        // Load the Oodle library (from `path`, or the default names if empty) and register the Oodle
        // decompressor. Returns true on success. Safe to call more than once; a successful load sticks.
        static bool Initialize(const std::string& path = "");

        static bool IsInitialized();
    };
}
