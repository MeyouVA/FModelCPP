// No C# counterpart: this is what stands in for a decoded Fmod5Sharp `FmodSample` throughout the port.
//
// In C# every path that reaches audio — EventNodesResolver, FModReader.ExtractTracks, the sound table, and
// FModProvider on top of them — ends at a FmodSample, an already-decoded PCM/Vorbis/etc. blob produced by
// the external Fmod5Sharp package. That decoder is not part of CUE4Parse's own source tree and is not
// ported (see FModSoundBank.h), so those paths end here instead: the *identity* of the sample, which is the
// (SoundBankIndex, SubsoundIndex) pair naming one subsound of one FSB5 container. The container's raw bytes
// are kept on FModSoundBank, so dropping a real FSB5 decoder in later is a local change.
//
// It lives in its own header because both FModReader.h and Utils/EventNodesResolver.h need it and the
// latter includes the former.
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Utils
{
    struct FWaveformRef
    {
        int32_t SoundBankIndex = 0;
        int32_t SubsoundIndex = 0;

        bool operator==(const FWaveformRef& o) const
        {
            return SoundBankIndex == o.SoundBankIndex && SubsoundIndex == o.SubsoundIndex;
        }
    };
}
