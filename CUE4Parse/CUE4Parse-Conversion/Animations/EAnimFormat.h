// Ported from CUE4Parse-Conversion/Animations/EAnimFormat.cs
#pragma once

namespace CUE4Parse_Conversion::Animations
{
    enum class EAnimFormat
    {
        ActorX,
        UEFormat
    };

    inline const char* Description(EAnimFormat value)
    {
        switch (value)
        {
            case EAnimFormat::ActorX:   return "ActorX (psa)";
            case EAnimFormat::UEFormat: return "UEFormat (ueanim)";
        }
        return "";
    }
}
