#include "USoundWave.h"

#include <exception>

#include "../PropertyUtil.h"
#include "../../../Versions/EGame.h"
#include "../../../Versions/FFrameworkObjectVersion.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;
    // FFrameworkObjectVersion is a namespace (C# static class), so it is brought in via the namespace itself.
    using namespace CUE4Parse::UE4::Versions;

    namespace
    {
        bool HasFlag(ESoundWaveFlag value, ESoundWaveFlag flag)
        {
            return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag);
        }
    }

    void USoundWave::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UObject::Deserialize(Ar, validPos);

        Subtitles = PropertyUtil::GetStructArray<FSubtitleCue>(*this, "Subtitles");

        // Nothing on disk says whether the payload is streamed; the version option is the starting guess and
        // either of these two properties overrides it.
        bStreaming = Ar.Versions["SoundWave.UseAudioStreaming"];
        bool s = false;
        FName loadingBehavior;
        if (PropertyUtil::TryGet(*this, "bStreaming", s)) // will return false if not found
        {
            bStreaming = s;
        }
        else if (PropertyUtil::TryGet(*this, "LoadingBehavior", loadingBehavior))
        {
            bStreaming = !loadingBehavior.IsNone() && loadingBehavior.Text() != "ESoundWaveLoadingBehavior::ForceInline";
            if (Ar.Game() == GAME_Stray && bStreaming)
                bStreaming = loadingBehavior.Text() != "ESoundWaveLoadingBehavior::RetainOnLoad";
        }

        const auto flags = Ar.Read<ESoundWaveFlag>();
        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::SOUND_COMPRESSION_TYPE_ADDED &&
            FFrameworkObjectVersion::Get(Ar) < FFrameworkObjectVersion::RemoveSoundWaveCompressionName)
        {
            Ar.ReadFName(); // DummyCompressionName
        }

        const bool bCooked = HasFlag(flags, ESoundWaveFlag::CookedFlag);

        if (Ar.Game() >= GAME_UE5_4 && bCooked)
        {
            SerializeCuePoints(Ar);
        }

        const int64_t saved = Ar.Position;
        try
        {
            SerializePlatformData(Ar, bCooked);
        }
        catch (const std::exception&)
        {
            // The streaming guess was wrong: undo everything it read and take the other branch.
            bStreaming = !bStreaming;
            Ar.Position = saved;
            CompressedFormatData.reset();
            RawData.reset();
            CompressedDataGuid = FGuid();
            RunningPlatformData.reset();
            SerializePlatformData(Ar, bCooked);
        }
    }

    void USoundWave::SerializePlatformData(Readers::FAssetArchive& Ar, bool bCooked)
    {
        if (!bStreaming)
        {
            if (bCooked)
            {
                CompressedFormatData.emplace(Ar);
            }
            else
            {
                RawData.emplace(Ar);
            }

            CompressedDataGuid = Ar.Read<FGuid>();
        }
        else
        {
            CompressedDataGuid = Ar.Read<FGuid>();
            if (bCooked)
                SerializeCookedPlatformData(Ar);
        }
    }

    void USoundWave::SerializeCuePoints(Readers::FAssetArchive& Ar)
    {
        // Count-prefixed; built in place because FStructFallback is move-only in this port.
        const int32_t count = Ar.Read<int32_t>();
        std::vector<FStructFallback> cuePoints;
        cuePoints.reserve(static_cast<size_t>(count < 0 ? 0 : count));
        for (int32_t i = 0; i < count; i++) cuePoints.emplace_back(Ar, std::optional<std::string>("SoundWaveCuePoint"));
        PlatformCuePoints = std::move(cuePoints);
    }

    void USoundWave::SerializeCookedPlatformData(Readers::FAssetArchive& Ar)
    {
        RunningPlatformData.emplace(Ar);
    }
}
