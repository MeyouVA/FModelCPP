// Ported from CUE4Parse-Conversion/UEFormat/Enums/EFileCompressionFormat.cs
#pragma once

namespace CUE4Parse_Conversion::UEFormat::Enums
{
    enum class EFileCompressionFormat
    {
        None,
        GZIP,
        ZSTD
    };

    inline const char* Description(EFileCompressionFormat value)
    {
        switch (value)
        {
            case EFileCompressionFormat::None: return "Uncompressed";
            case EFileCompressionFormat::GZIP: return "Gzip Compression";
            case EFileCompressionFormat::ZSTD: return "ZStandard Compression";
        }
        return "";
    }
}
