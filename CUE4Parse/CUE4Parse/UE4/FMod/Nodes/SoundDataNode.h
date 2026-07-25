// Ported from CUE4Parse/UE4/FMod/Nodes/SoundDataNode.cs
// A SND chunk: an embedded FSB5 audio container. C# hands the (optionally decrypted) FSB5 stream to
// Fmod5Sharp's FsbLoader to decode the samples. That external decoder is out of scope here (see
// FModSoundBank.h), so the port records the raw FSB5 bytes and the sub-sound count from the header, and
// leaves the sample decode out -- mirroring how the Wwise port keeps FDeferredByteData for WEM audio.
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../Fsb5Decryption.h"
#include "../FModSoundBank.h"
#include "../Metadata/SoundDataInfo.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class SoundDataNode
    {
    public:
        std::optional<FModSoundBank> SoundBank;

        SoundDataNode(Readers::FArchive& Ar, int64_t nodeStart, uint32_t size, int soundDataIndex)
        {
            uint32_t fsbOffset = FModReader::SoundDataInfo->Header[soundDataIndex].FSBOffset;

            Ar.Position = fsbOffset;
            std::vector<uint8_t> data = Ar.ReadBytes(static_cast<int>(size));

            // In case FSB5 is encrypted: bit-reverse + XOR with the key, then re-check the header.
            if (!Fsb5Decryption::IsFSB5Header(data))
            {
                if (!FModReader::EncryptionKey.has_value() || FModReader::EncryptionKey->empty())
                {
                    // C# throws in this case and the caller's try/catch leaves SoundBank null; do the same.
                    Ar.Position = nodeStart + 8 + size;
                    return;
                }
                Fsb5Decryption::DecryptInPlace(data, *FModReader::EncryptionKey);
                if (!Fsb5Decryption::IsFSB5Header(data))
                {
                    // Wrong key -- failed to decrypt; leave SoundBank null (C# throws & the caller catches).
                    Ar.Position = nodeStart + 8 + size;
                    return;
                }
            }

            FModSoundBank bank;
            bank.SampleCount = ParseSampleCount(data);
            bank.Data = std::move(data);
            SoundBank = std::move(bank);

            // Matches C#'s final seek (fsbOffset - relativeOffset + size == nodeStart + 8 + size).
            Ar.Position = nodeStart + 8 + size;
        }

    private:
        // FSB5 header: "FSB5"(4), version(4), numSamples(4, little-endian) ...
        static int32_t ParseSampleCount(const std::vector<uint8_t>& data)
        {
            if (data.size() < 12) return 0;
            return static_cast<int32_t>(
                static_cast<uint32_t>(data[8]) |
                (static_cast<uint32_t>(data[9]) << 8) |
                (static_cast<uint32_t>(data[10]) << 16) |
                (static_cast<uint32_t>(data[11]) << 24));
        }
    };
}
