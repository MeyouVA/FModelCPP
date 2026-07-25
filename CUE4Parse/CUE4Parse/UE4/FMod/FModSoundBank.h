// Stand-in for Fmod5Sharp's FmodSoundBank.
//
// The C# reader decodes each SND chunk's embedded FSB5 container into a FmodSoundBank whose .Samples
// are fully decoded PCM/Vorbis/etc. samples, using the external Fmod5Sharp NuGet library. That decoder
// is not part of CUE4Parse's own source tree, so -- exactly as the Wwise port keeps FDeferredByteData
// instead of decoding WEM audio -- this port records the raw FSB5 bytes and leaves the sample decode out.
//
// Consumers that reached into bank.Samples[i] in C# (EventNodesResolver, the sound-table lookup) instead
// work with (SoundBankIndex, SubsoundIndex) waveform references here; the raw bytes are retained so a
// real FSB5 decoder can be dropped in later.
#pragma once

#include <cstdint>
#include <vector>

namespace CUE4Parse::UE4::FMod
{
    struct FModSoundBank
    {
        // The raw (already decrypted, if it was encrypted) FSB5 container bytes.
        std::vector<uint8_t> Data;
        // The FSB5 sub-sound count, parsed from the container header (0 if the header wasn't recognised).
        int32_t SampleCount = 0;

        FModSoundBank() = default;
    };
}
