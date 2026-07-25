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
        // Wrap a cloned base so the clone is independent, but carry the payload map over by reference —
        // C# does the same, and payloads are only ever registered during package initialization.
        auto clone = std::unique_ptr<FAssetArchive>(
            new FAssetArchive(_baseArchive->Clone(), Owner, AbsoluteOffset, _payloads));
        clone->Position = Position;
        return clone;
    }

    std::unique_ptr<FAssetArchive> FAssetArchive::GetPayload(PayloadType type,
                                                            const Objects::FByteBulkDataHeader* header) const
    {
        const auto it = _payloads->find(type);
        std::unique_ptr<FAssetArchive> reader;
        if (it != _payloads->end() && it->second) reader = it->second(header);
        if (reader == nullptr)
            throw Exceptions::ParserException(*this,
                "Requested payload of type " + std::string(Utils::ToExtension(type)) + " was not found");
        return reader;
    }

    std::unique_ptr<FAssetArchive> FAssetArchive::TryGetPayload(PayloadType type,
                                                               const Objects::FByteBulkDataHeader* header) const
    {
        try
        {
            return GetPayload(type, header);
        }
        catch (...)
        {
            return nullptr;
        }
    }

    void FAssetArchive::AddPayload(PayloadType type, int absoluteOffset, RawPayloadProvider payload)
    {
        if (_payloads->count(type) != 0)
            throw Exceptions::ParserException(*this,
                "Can't add a payload that is already attached of type " + std::string(Utils::ToExtension(type)));

        IPackage* owner = Owner;
        (*_payloads)[type] = [payload = std::move(payload), owner, absoluteOffset](const Objects::FByteBulkDataHeader* header)
            -> std::unique_ptr<FAssetArchive>
        {
            auto rawAr = payload(header);
            if (rawAr == nullptr) return nullptr;
            return std::unique_ptr<FAssetArchive>(new FAssetArchive(std::move(rawAr), owner, absoluteOffset, nullptr));
        };
    }

    void FAssetArchive::AddPayload(PayloadType type, std::shared_ptr<FAssetArchive> payload)
    {
        if (_payloads->count(type) != 0)
            throw Exceptions::ParserException(*this,
                "Can't add a payload that is already attached of type " + std::string(Utils::ToExtension(type)));

        (*_payloads)[type] = [payload = std::move(payload)](const Objects::FByteBulkDataHeader*)
            -> std::unique_ptr<FAssetArchive>
        {
            if (payload == nullptr) return nullptr;
            auto clone = payload->Clone();
            return std::unique_ptr<FAssetArchive>(static_cast<FAssetArchive*>(clone.release()));
        };
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
