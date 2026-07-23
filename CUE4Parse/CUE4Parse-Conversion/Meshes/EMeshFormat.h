// Ported from CUE4Parse-Conversion/Meshes/EMeshFormat.cs
#pragma once

namespace CUE4Parse_Conversion::Meshes
{
    enum class EMeshFormat
    {
        ActorX,
        Gltf2,
        OBJ,
        UEFormat
    };

    inline const char* Description(EMeshFormat value)
    {
        switch (value)
        {
            case EMeshFormat::ActorX:   return "ActorX (psk / pskx)";
            case EMeshFormat::Gltf2:    return "glTF 2.0 (binary)";
            case EMeshFormat::OBJ:      return "Wavefront OBJ (Not Implemented)";
            case EMeshFormat::UEFormat: return "UEFormat (uemodel)";
        }
        return "";
    }
}
