// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTextureMovie.cs
// RawData is a video in bnk format (https://www.radgametools.com/bnkdown.htm)
#pragma once

#include <cstdint>
#include <optional>

#include "UTexture.h"
#include "../../Objects/FByteBulkData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;

    class UTextureMovie : public UTexture
    {
    public:
        std::optional<FByteBulkData> RawData;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UTexture::Deserialize(Ar, validPos);
            RawData.emplace(Ar);
        }
    };
}
