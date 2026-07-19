// Ported from CUE4Parse/UE4/Assets/Readers/FAssetArchive.cs (out-of-line members).
#include "FAssetArchive.h"

#include <stdexcept>

#include "../../Versions/ObjectVersion.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets::Readers
{
    using namespace CUE4Parse::UE4::Versions;

    int FAssetArchive::Read(uint8_t* buffer, int offset, int count)
    {
        // Treat the base as a random-access store addressed by this archive's Position.
        const int n = _baseArchive->ReadAt(Position, buffer, offset, count);
        Position += n;
        return n;
    }

    int64_t FAssetArchive::Seek(int64_t offset, ESeekOrigin origin)
    {
        switch (origin)
        {
            case ESeekOrigin::Begin:   Position = offset; break;
            case ESeekOrigin::Current: Position = Position + offset; break;
            case ESeekOrigin::End:     Position = Length + offset; break;
            default: throw std::out_of_range("Invalid SeekOrigin");
        }
        return Position;
    }

    int64_t FAssetArchive::SeekAbsolute(int64_t offset, ESeekOrigin origin)
    {
        return Seek(offset - AbsoluteOffset, origin);
    }

    std::unique_ptr<FArchive> FAssetArchive::Clone() const
    {
        // Wrap a cloned base so the clone is independent (payloads are deferred, so nothing else to carry).
        auto clone = std::unique_ptr<FAssetArchive>(new FAssetArchive(_baseArchive->Clone(), Owner, AbsoluteOffset));
        clone->Position = Position;
        return clone;
    }

    FName FAssetArchive::ReadFName()
    {
        const int nameIndex = Read<int32_t>();
        int extraIndex = 0;
        if (Ver() >= EUnrealEngineObjectUE3Version::FNAME_CHANGE_NAME_SPLIT)
            extraIndex = Read<int32_t>();

        if (Owner == nullptr)
            throw Exceptions::ParserException(*this, "FName could not be read: archive has no owning package");
        const auto& nameMap = Owner->NameMap();
        if (nameIndex < 0 || nameIndex >= static_cast<int>(nameMap.size()))
            throw Exceptions::ParserException(*this,
                "FName could not be read, requested index " + std::to_string(nameIndex) +
                ", name map size " + std::to_string(nameMap.size()));

        return FName(nameMap, nameIndex, extraIndex);
    }

    bool FAssetArchive::TestReadFName()
    {
        if (HasUnversionedProperties()) return false;
        const int64_t savedPos = Position;
        if (Position + 2 * static_cast<int64_t>(sizeof(int32_t)) >= Length) return false;
        const int nameIndex = Read<int32_t>();
        const int index = Read<int32_t>();
        Position = savedPos;
        return Owner != nullptr && nameIndex >= 0 && nameIndex < static_cast<int>(Owner->NameMap().size()) && index >= 0 && index < 256;
    }
}
