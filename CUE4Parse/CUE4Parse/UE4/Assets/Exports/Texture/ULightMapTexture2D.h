// Ported from CUE4Parse/UE4/Assets/Exports/Texture/ULightMapTexture2D.cs
// A baked lightmap. Identical to a 2D texture except for one trailing flags word after everything else.
//
// Deliberate difference from C#: WriteJson (which prints LightmapFlags as a bitfield string) is dropped --
// the port has no JSON serializer layer.
#pragma once

#include <cstdint>

#include "UTexture2D.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    // [Flags]
    enum class ELightMapFlags : int32_t
    {
        LMF_None       = 0,          // No flags
        LMF_Streamed   = 0x00000001, // Lightmap should be placed in a streaming texture
        LMF_LQLightmap = 0x00000002  // Whether this is a low quality lightmap or not
    };

    class ULightMapTexture2D : public UTexture2D
    {
    public:
        ELightMapFlags LightmapFlags = ELightMapFlags::LMF_None;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UTexture2D::Deserialize(Ar, validPos);

            LightmapFlags = Ar.Read<ELightMapFlags>();
        }
    };
}
