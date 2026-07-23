// Ported from CUE4Parse/UE4/Assets/Exports/Nanite/ENaniteMeshFormat.cs
#pragma once

namespace CUE4Parse::UE4::Assets::Exports::Nanite
{
    enum class ENaniteMeshFormat
    {
        OnlyNaniteLOD,
        OnlyNormalLODs,
        AllLayersNaniteFirst,
        AllLayersNaniteLast,
    };

    inline const char* Description(ENaniteMeshFormat value)
    {
        switch (value)
        {
            case ENaniteMeshFormat::OnlyNaniteLOD:        return "Only Nanite LOD";
            case ENaniteMeshFormat::OnlyNormalLODs:       return "Only Normal LODs";
            case ENaniteMeshFormat::AllLayersNaniteFirst: return "Nanite LOD first, then Normal LODs";
            case ENaniteMeshFormat::AllLayersNaniteLast:  return "Normal LODs first, then Nanite LOD";
        }
        return "";
    }
}
