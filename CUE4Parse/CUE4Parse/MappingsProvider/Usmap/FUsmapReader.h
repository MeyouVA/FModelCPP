// Ported from CUE4Parse/MappingsProvider/Usmap/FUsmapReader.cs
// A delegating FArchive that carries the usmap file's EUsmapVersion alongside the raw reader — the parse
// helpers branch on Version while reading.
//
// Deliberate difference from C#: like FAssetArchive, this port keeps Position/Length as its own state and
// treats the inner archive as a random-access byte store (ReadAt) instead of delegating the properties.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "UsmapEnums.h"
#include "../../UE4/Readers/FArchive.h"

namespace CUE4Parse::MappingsProvider::Usmap
{
    using CUE4Parse::UE4::Readers::FArchive;
    using CUE4Parse::UE4::Readers::ESeekOrigin;

    class FUsmapReader : public FArchive
    {
    public:
        EUsmapVersion Version;

        FUsmapReader(FArchive& innerArchive, EUsmapVersion version)
            : FArchive(innerArchive.Versions), Version(version), _inner(&innerArchive)
        {
            Length = innerArchive.Length;
            Position = innerArchive.Position;
        }

        // The byte-source override hides the base's typed Read<T>; `using` restores it (same pattern as
        // FAssetArchive).
        using FArchive::Read;
        int Read(uint8_t* buffer, int offset, int count) override
        {
            const int n = _inner->ReadAt(Position, buffer, offset, count);
            Position += n;
            return n;
        }

        int64_t Seek(int64_t offset, ESeekOrigin origin) override
        {
            switch (origin)
            {
                case ESeekOrigin::Begin:   Position = offset; break;
                case ESeekOrigin::Current: Position = Position + offset; break;
                case ESeekOrigin::End:     Position = Length + offset; break;
            }
            return Position;
        }

        bool CanSeek() const override { return _inner->CanSeek(); }
        const std::string& Name() const override { return _inner->Name(); }

        std::unique_ptr<FArchive> Clone() const override
        {
            auto ownedInner = _inner->Clone();
            auto clone = std::make_unique<FUsmapReader>(*ownedInner, Version);
            clone->_ownedInner = std::move(ownedInner);
            clone->_inner = clone->_ownedInner.get();
            clone->Position = Position;
            return clone;
        }

    private:
        FArchive* _inner;
        std::unique_ptr<FArchive> _ownedInner; // set on clones only
    };
}
