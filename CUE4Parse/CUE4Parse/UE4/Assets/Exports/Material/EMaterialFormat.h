// Ported from CUE4Parse/UE4/Assets/Exports/Material/EMaterialFormat.cs
#pragma once

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    enum class EMaterialFormat
    {
        FirstLayer,
        AllLayersNoRef,
        AllLayers
    };

    inline const char* Description(EMaterialFormat value)
    {
        switch (value)
        {
            case EMaterialFormat::FirstLayer:     return "First Layer Only";
            case EMaterialFormat::AllLayersNoRef: return "All Layers (Without Referenced Textures)";
            case EMaterialFormat::AllLayers:      return "All Layers (With All Referenced Textures)";
        }
        return "";
    }
}
