// Ported from CUE4Parse/UE4/Wwise/Enums/EAkCompanyID.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // From SDK
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class AkCompanyID : uint16_t
    {
        PluginDevMin         = 64,
        PluginDevMax         = 255,
        // Audiokinetic inc.
        Audiokinetic         = 0,
        // Audiokinetic inc.
        AudiokineticExternal = 1,
        // McDSP
        McDsp                = 256,
        // WaveArts
        WaveArts             = 257,
        // Phonetic Arts
        PhoneticArts         = 258,
        // iZotope
        Izotope              = 259,
        // Crankcase Audio
        CrankcaseAudio       = 261,
        // IOSONO
        Iosono               = 262,
        // Auro Technologies
        AuroTechnologies     = 263,
        // Dolby
        Dolby                = 264,
        // Two Big Ears
        TwoBigEars           = 265,
        // Oculus
        Oculus               = 266,
        // Blue Ripple Sound
        BlueRippleSound      = 267,
        // Enzien Audio
        Enzien               = 268,
        // Krotos (Dehumanizer)
        Krotos               = 269,
        // Nurulize
        Nurulize             = 270,
        // Super Powered
        SuperPowered         = 271,
        // Google
        Google               = 272,
        // The following are commented out in the source to avoid redefinition:
        Nvidia               = 273,
        Reserved             = 274,
        Microsoft            = 275,
        Yamaha               = 276,
        // Visisonics
        Visisonics           = 277,
    };
}
