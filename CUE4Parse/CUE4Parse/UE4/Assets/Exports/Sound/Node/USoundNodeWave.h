// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/USoundNodeWave.cs
// The UE3-era sound wave: the payload was a per-platform pile of bulk-data blobs before UE4 folded them into
// USoundWave. Every read here is gated on a UE3 package version.
#pragma once

#include <optional>

#include "../../UObject.h"
#include "../../../Objects/FByteBulkData.h"
#include "../../../Readers/FAssetArchive.h"
#include "../../../../Objects/Core/Misc/FGuid.h"
#include "../../../../Objects/UObject/FFormatContainer.h"
#include "../../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::UObject::FFormatContainer;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE3Version;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;

    class USoundNodeWave : public UObject
    {
    public:
        std::optional<FFormatContainer> CompressedFormatData;
        std::optional<FByteBulkData> RawSound;
        std::optional<FByteBulkData> PCSound;
        std::optional<FByteBulkData> XboxSound;
        std::optional<FByteBulkData> PS3Sound;
        std::optional<FByteBulkData> WIIUSound;
        std::optional<FByteBulkData> IPhoneSound;
        std::optional<FByteBulkData> FlashSound;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            const bool bCooked = Ar.Ver() > EUnrealEngineObjectUE4Version::ADD_COOKED_TO_SOUND_NODE_WAVE && Ar.ReadBoolean();

            if (Ar.Ver() < EUnrealEngineObjectUE3Version::ADDED_CACHED_COOKED_PC_DATA)
            {
                Ar.ReadFName(); // FileType
            }

            if (Ar.Ver() >= EUnrealEngineObjectUE3Version::UPDATED_SOUND_NODE_WAVE &&
                Ar.Ver() < EUnrealEngineObjectUE3Version::CLEANUP_SOUNDNODEWAVE)
            {
                Ar.SkipFixedArray(sizeof(int32_t)); // ChannelOffsets
                Ar.SkipFixedArray(sizeof(int32_t)); // ChannelSizes
            }

            if (bCooked)
            {
                CompressedFormatData.emplace(Ar);
            }
            else
            {
                RawSound.emplace(Ar);
            }

            if (Ar.Ver() >= EUnrealEngineObjectUE3Version::ADDED_RAW_SURROUND_DATA &&
                Ar.Ver() < EUnrealEngineObjectUE3Version::UPDATED_SOUND_NODE_WAVE)
            {
                // Read for the cursor only, as in C#.
                const int32_t count = Ar.Read<int32_t>();
                for (int32_t i = 0; i < count; i++) (void) FByteBulkData(Ar);
            }

            if (Ar.Ver() >= EUnrealEngineObjectUE3Version::ADDED_NUM_CHANNELS &&
                Ar.Ver() < EUnrealEngineObjectUE3Version::CLEANUP_SOUNDNODEWAVE)
            {
                Ar.Read<int32_t>(); // ChannelCount
            }

            if (Ar.Ver() < EUnrealEngineObjectUE4Version::ADD_SOUNDNODEWAVE_TO_DDC)
            {
                if (Ar.Ver() >= EUnrealEngineObjectUE3Version::ADDED_CACHED_COOKED_PC_DATA)
                {
                    PCSound.emplace(Ar);
                }

                if (Ar.Ver() >= EUnrealEngineObjectUE3Version::ADDED_CACHED_COOKED_XBOX360_DATA)
                {
                    XboxSound.emplace(Ar);
                }

                if (Ar.Ver() >= EUnrealEngineObjectUE3Version::ADDED_CACHED_COOKED_PS3_DATA)
                {
                    PS3Sound.emplace(Ar);
                }

                if (Ar.Ver() >= EUnrealEngineObjectUE3Version::WIIU_COMPRESSED_SOUNDS)
                {
                    WIIUSound.emplace(Ar);
                }

                if (Ar.Ver() >= EUnrealEngineObjectUE3Version::IPHONE_COMPRESSED_SOUNDS)
                {
                    IPhoneSound.emplace(Ar);
                }

                if (Ar.Ver() >= EUnrealEngineObjectUE3Version::FLASH_MERGE_TO_MAIN)
                {
                    FlashSound.emplace(Ar);
                }
            }

            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::ADD_SOUNDNODEWAVE_GUID)
            {
                Ar.Read<FGuid>(); // CompressedDataGuid
            }
        }
    };
}
