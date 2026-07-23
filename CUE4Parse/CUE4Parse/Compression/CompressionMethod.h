// Ported from CUE4Parse/Compression/CompressionMethod.cs
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::Compression
{
    // Values match the C# enum exactly (the numbering gap after Oodle=4 is implicit-sequential in C#).
    enum class CompressionMethod : int32_t
    {
        None = 0,
        Zlib = 1,
        Gzip = 2, //???
        Custom = 3,
        Oodle = 4,
        LZ4,
        LZO,
        Zstd,
        XB1Zlib,
        XboxOneGDKZlib,
        Brotli,
        PWC, // Century: Age of Ashes (custom obfuscation)
        Unknown
    };

    // Stands in for Enum.TryParse<CompressionMethod>(name, ignoreCase, out method). C++ has no enum
    // reflection, so the names are listed once here, in declaration order. FPakInfo parses the FName-based
    // compression method strings out of a pak trailer case-insensitively; the bulk-data header parser in
    // FArchive matches C#'s case-sensitive default.
    inline bool TryParseCompressionMethod(const std::string& name, CompressionMethod& method, bool ignoreCase = false)
    {
        static const struct { const char* Name; CompressionMethod Value; } table[] = {
            {"None", CompressionMethod::None},
            {"Zlib", CompressionMethod::Zlib},
            {"Gzip", CompressionMethod::Gzip},
            {"Custom", CompressionMethod::Custom},
            {"Oodle", CompressionMethod::Oodle},
            {"LZ4", CompressionMethod::LZ4},
            {"LZO", CompressionMethod::LZO},
            {"Zstd", CompressionMethod::Zstd},
            {"XB1Zlib", CompressionMethod::XB1Zlib},
            {"XboxOneGDKZlib", CompressionMethod::XboxOneGDKZlib},
            {"Brotli", CompressionMethod::Brotli},
            {"PWC", CompressionMethod::PWC},
            {"Unknown", CompressionMethod::Unknown},
        };

        for (const auto& entry : table)
        {
            const char* candidate = entry.Name;
            size_t i = 0;
            for (; candidate[i] != '\0' && i < name.size(); ++i)
            {
                char a = candidate[i], b = name[i];
                if (ignoreCase)
                {
                    if (a >= 'A' && a <= 'Z') a = static_cast<char>(a + 32);
                    if (b >= 'A' && b <= 'Z') b = static_cast<char>(b + 32);
                }
                if (a != b) break;
            }
            if (candidate[i] == '\0' && i == name.size())
            {
                method = entry.Value;
                return true;
            }
        }
        return false;
    }
}
