// Ported from CUE4Parse-Conversion/Meshes/ESocketFormat.cs
#pragma once

namespace CUE4Parse_Conversion::Meshes
{
    enum class ESocketFormat
    {
        Socket,
        Bone,
        None
    };

    inline const char* Description(ESocketFormat value)
    {
        switch (value)
        {
            case ESocketFormat::Socket: return "Export Bone Sockets in a Separate Header (SKELSOCK)";
            case ESocketFormat::Bone:   return "Export Bone Sockets as Bones";
            case ESocketFormat::None:   return "Don't Export Bone Sockets";
        }
        return "";
    }
}
