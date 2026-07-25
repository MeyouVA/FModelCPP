// Ported from CUE4Parse/UE4/Wwise/WwiseArchive.cs
// A thin decorator over another FArchive that carries the two pieces of state every Wwise reader needs:
// the bank Version and the HasFeedback flag, both read out of the BankHeader section. Practically every
// object in UE4/Wwise branches on Ar.Version, so it is threaded through the archive rather than passed down.
//
// Differences from the C# version, by design:
//   * C# overrides the Position property to forward to the inner archive, so the wrapper and the inner
//     genuinely share one cursor. In this port Position is a plain field on FArchive (see the note at the
//     top of FArchive.h), so the wrapper keeps its own and syncs it into the inner around every read.
//     Everything routes through the virtual Read()/Seek(), so overriding those two covers all the
//     inherited helpers (ReadBytes -> Serialize -> ReadScalar/ReadElements, ReadSpan, ...).
//   * Following from that, Clone() yields an archive with an independent cursor. The C# clone shares the
//     inner archive and therefore shares the position; matching that here would mean reintroducing the
//     virtual-Position design the port deliberately dropped.
#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../Readers/FArchive.h"
#include "../Readers/FByteArchive.h"
#include "WwiseVersionInfo.h"

namespace CUE4Parse::UE4::Wwise
{
    using CUE4Parse::UE4::Readers::FArchive;
    using CUE4Parse::UE4::Readers::FByteArchive;
    using CUE4Parse::UE4::Readers::ESeekOrigin;
    using CUE4Parse::UE4::Versions::VersionContainer;

    class FWwiseArchive final : public FArchive
    {
    public:
        /// Wwise version, read from the BankHeader section of the .bnk file
        /// Can also be deducted from plugin version
        uint32_t Version = 0;

        /// Read from BankHeader section of the .bnk file
        /// Only relevant for versions <= 126
        bool HasFeedback = false;

        // Wraps an existing archive without taking ownership of it (C#'s primary constructor).
        explicit FWwiseArchive(FArchive& archive)
            : FArchive(archive.Versions), _archive(&archive)
        {
            Position = archive.Position;
            Length = archive.Length;
        }

        FWwiseArchive(std::string name, std::vector<uint8_t> data, VersionContainer versions = VersionContainer())
            : FArchive(versions), _owned(std::make_unique<FByteArchive>(std::move(name), std::move(data), std::move(versions)))
        {
            _archive = _owned.get();
            Length = _archive->Length;
        }

        // Overriding the byte-source Read() would otherwise hide the inherited Read<T>() template --
        // `Ar.Read<uint32_t>()` would parse `<` as less-than and fail. FByteArchive avoids this by
        // redeclaring the templates; here the base ones are correct, so pull them back into scope.
        using FArchive::Read;

        int Read(uint8_t* buffer, int offset, int count) override
        {
            _archive->Position = Position;
            const int read = _archive->Read(buffer, offset, count);
            Position = _archive->Position;
            return read;
        }

        int64_t Seek(int64_t offset, ESeekOrigin origin) override
        {
            _archive->Position = Position;
            Position = _archive->Seek(offset, origin);
            return Position;
        }

        bool CanSeek() const override { return _archive->CanSeek(); }
        const std::string& Name() const override { return _archive->Name(); }

        std::unique_ptr<FArchive> Clone() const override
        {
            auto clone = std::make_unique<FWwiseArchive>(*_archive);
            clone->Version = Version;
            clone->Position = Position;
            return clone;
        }

        bool IsSupported() const { return CUE4Parse::UE4::Wwise::IsSupported(Version); }

        // A NUL-terminated UTF-8 string with no length prefix ("stz"). Used for platform names and the
        // BankInit plugin list from version 137 on, where the old length-prefixed FString was dropped.
        std::string ReadStzString()
        {
            std::string result;
            result.reserve(16);
            while (true)
            {
                const uint8_t b = Read<uint8_t>();
                if (b == 0) break;
                result.push_back(static_cast<char>(b));

                if (result.size() >= 255)
                    throw std::invalid_argument("ReadStz: string too long (no terminator within 255 bytes).");
            }
            return result;
        }

        // Big-endian variable-length int: unlike FArchive::Read7BitEncodedInt the payload bits accumulate
        // most-significant first, so this is not interchangeable with it.
        int Read7BitEncodedIntBE()
        {
            int max = 0;

            uint8_t cur = Read<uint8_t>();
            int value = cur & 0x7F;

            while ((cur & 0x80) != 0)
            {
                if (++max >= 10)
                    throw std::runtime_error("Unexpected variable loop count");

                cur = Read<uint8_t>();
                value = (value << 7) | (cur & 0x7F);
            }

            return value;
        }

        // A one-byte bool. FArchive::ReadBoolean reads a *four*-byte one, so the two are not the same call.
        bool ReadBool() { return Read<uint8_t>() != 0; }

    private:
        std::unique_ptr<FArchive> _owned; // only set by the (name, data) constructor
        FArchive* _archive = nullptr;
    };
}
