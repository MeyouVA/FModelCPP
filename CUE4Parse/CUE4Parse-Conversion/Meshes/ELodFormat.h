// Ported from CUE4Parse-Conversion/Meshes/ELodFormat.cs
#pragma once

namespace CUE4Parse_Conversion::Meshes
{
    enum class ELodFormat
    {
        FirstLod,
        AllLods
    };

    inline const char* Description(ELodFormat value)
    {
        switch (value)
        {
            case ELodFormat::FirstLod: return "First Level Only";
            case ELodFormat::AllLods:  return "All Levels";
        }
        return "";
    }
}
