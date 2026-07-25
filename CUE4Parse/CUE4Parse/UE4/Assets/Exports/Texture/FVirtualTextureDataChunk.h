// Ported from CUE4Parse/UE4/Assets/Exports/Texture/FVirtualTextureDataChunk.cs
// One streamed chunk of a virtual texture: per-layer codec selection plus the bulk payload holding the
// tiles. As everywhere else in this tree, the payload stays lazy.
//
// Deliberate difference from C#: the [JsonConverter] attribute is dropped (no JSON serializer layer).
#pragma once

#include <cstdint>
#include <vector>

#include "../../Objects/FByteBulkData.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Core/Misc/FSHAHash.h"
#include "../../../Versions/EGame.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Objects::Core::Misc::FSHAHash;
    using namespace CUE4Parse::UE4::Versions;

    enum class EVirtualTextureCodec : uint8_t
    {
        Black,                  //Special case codec, always outputs black pixels 0,0,0,0
        OpaqueBlack,            //Special case codec, always outputs opaque black pixels 0,0,0,255
        White,                  //Special case codec, always outputs white pixels 255,255,255,255
        Flat,                   //Special case codec, always outputs 128,125,255,255 (flat normal map)
        RawGPU,                 //Uncompressed data in an GPU-ready format (e.g R8G8B8A8, BC7, ASTC, ...)
        ZippedGPU_DEPRECATED,   //Same as RawGPU but with the data zipped
        Crunch_DEPRECATED,      //Use the Crunch library to compress data
        Max,                    // Add new codecs before this entry
    };

    class FVirtualTextureDataChunk
    {
    public:
        FByteBulkData BulkData;
        uint32_t SizeInBytes = 0;
        uint32_t CodecPayloadSize = 0;
        std::vector<uint32_t> CodecPayloadOffset;
        std::vector<EVirtualTextureCodec> CodecType;

        FVirtualTextureDataChunk(Readers::FAssetArchive& Ar, uint32_t numLayers)
        {
            CodecType.assign(numLayers, EVirtualTextureCodec::Black);
            CodecPayloadOffset.assign(numLayers, 0u);
            if (Ar.Game() >= GAME_UE5_0)
                Ar.Position += FSHAHash::SIZE; // var bulkDataHash = new FSHAHash(Ar);

            SizeInBytes = Ar.Read<uint32_t>();
            CodecPayloadSize = Ar.Read<uint32_t>();
            for (uint32_t layerIndex = 0u; layerIndex < numLayers; ++layerIndex)
            {
                CodecType[layerIndex] = Ar.Read<EVirtualTextureCodec>();
                if (Ar.Game() == GAME_DeltaForce) continue;
                CodecPayloadOffset[layerIndex] = Ar.Game() >= GAME_UE4_27 ? Ar.Read<uint32_t>() : Ar.Read<uint16_t>();
            }
            BulkData = FByteBulkData(Ar);
        }
    };
}
