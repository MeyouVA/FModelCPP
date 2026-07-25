// Ported from CUE4Parse/UE4/Assets/Exports/Sound/USoundWave.cs
// A cooked sound wave. The interesting part is that whether the payload is streamed or inlined is not
// reliably recorded: C# guesses from a version option plus two properties, and if the guess parses wrong it
// rewinds, flips the guess and re-reads. That retry is preserved verbatim.
//
// Deliberate difference from C#: WriteJson is dropped (the port has no JSON serializer layer).
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "FStreamedAudioPlatformData.h"
#include "USoundBase.h"
#include "../../Objects/FByteBulkData.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Core/Misc/FGuid.h"
#include "../../../Objects/Engine/FSubtitleCue.h"
#include "../../../Objects/UObject/FFormatContainer.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::Engine::FSubtitleCue;
    using CUE4Parse::UE4::Objects::UObject::FFormatContainer;

    // [Flags]
    enum class ESoundWaveFlag : uint32_t
    {
        CookedFlag                  = 1 << 0,
        HasOwnerLoadingBehaviorFlag = 1 << 1,
        LoadingBehaviorShift        = 2,
        LoadingBehaviorMask         = 0b00000111,
    };

    class USoundWave : public USoundBase
    {
    public:
        std::vector<FSubtitleCue> Subtitles;
        bool bStreaming = true;
        std::optional<FFormatContainer> CompressedFormatData;
        std::optional<FByteBulkData> RawData;
        FGuid CompressedDataGuid;
        std::optional<FStreamedAudioPlatformData> RunningPlatformData;
        std::optional<std::vector<FStructFallback>> PlatformCuePoints;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;

    protected:
        virtual void SerializeCuePoints(Readers::FAssetArchive& Ar);
        virtual void SerializeCookedPlatformData(Readers::FAssetArchive& Ar);

        // C#'s SoundBaseDeserialize: skip this class's body and run only the UObject one. USoundWaveProcedural
        // uses it to opt out of the wave payload entirely.
        void SoundBaseDeserialize(Readers::FAssetArchive& Ar, int64_t validPos) { UObject::Deserialize(Ar, validPos); }

    private:
        void SerializePlatformData(Readers::FAssetArchive& Ar, bool bCooked);
    };
}
