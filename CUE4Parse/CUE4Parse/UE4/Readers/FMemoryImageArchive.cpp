#include "FMemoryImageArchive.h"

#include <utility>

namespace CUE4Parse::UE4::Readers
{
    using CUE4Parse::UE4::Objects::UObject::FName;

    FFrozenMemoryImagePtr::FFrozenMemoryImagePtr(FMemoryImageArchive& Ar)
    {
        Packed = Ar.Read<uint64_t>();
        IsFrozen = (Packed & 1) != 0;
        if (Ar.Game() >= Versions::GAME_UE5_0)
        {
            OffsetFromThis = static_cast<int64_t>(Packed) >> 24;
            TypeIndex = static_cast<int32_t>((Packed & TypeIndexMask) >> 1) - 1;
        }
        else
        {
            OffsetFromThis = static_cast<int64_t>(Packed) >> 1;
        }
    }

    FMemoryImageArchive::FMemoryImageArchive(std::shared_ptr<FArchive> ar)
        : FArchive(ar->Versions), InnerArchive(std::move(ar))
    {
        Position = InnerArchive->Position;
        Length = InnerArchive->Length;
    }

    FMemoryImageArchive::FMemoryImageArchive(std::shared_ptr<FArchive> ar, int arrayAlign)
        : FMemoryImageArchive(std::move(ar))
    {
        _arrayAlign = arrayAlign;
    }

    int FMemoryImageArchive::Read(uint8_t* buffer, int offset, int count)
    {
        InnerArchive->Position = Position;
        const int read = InnerArchive->Read(buffer, offset, count);
        Position = InnerArchive->Position;
        return read;
    }

    int64_t FMemoryImageArchive::Seek(int64_t offset, ESeekOrigin origin)
    {
        InnerArchive->Position = Position;
        const int64_t result = InnerArchive->Seek(offset, origin);
        Position = InnerArchive->Position;
        return result;
    }

    std::unique_ptr<FArchive> FMemoryImageArchive::Clone() const
    {
        auto clone = std::make_unique<FMemoryImageArchive>(std::shared_ptr<FArchive>(InnerArchive->Clone()), _arrayAlign);
        clone->Names = Names;
        clone->Position = Position;
        return clone;
    }

    std::vector<uint8_t> FMemoryImageArchive::ReadBytes(int length)
    {
        InnerArchive->Position = Position;
        std::vector<uint8_t> data = InnerArchive->ReadBytes(length);
        Position = InnerArchive->Position;
        return data;
    }

    void FMemoryImageArchive::Serialize(uint8_t* ptr, int length)
    {
        InnerArchive->Position = Position;
        InnerArchive->Serialize(ptr, length);
        Position = InnerArchive->Position;
    }

    void FMemoryImageArchive::ReadScalar(void* dst, int size)
    {
        InnerArchive->Position = Position;
        InnerArchive->ReadScalar(dst, size);
        Position = InnerArchive->Position;
    }

    void FMemoryImageArchive::ReadElements(void* dst, int elementSize, int count)
    {
        InnerArchive->Position = Position;
        InnerArchive->ReadElements(dst, elementSize, count);
        Position = InnerArchive->Position;
    }

    std::span<const uint8_t> FMemoryImageArchive::ReadSpan(int length)
    {
        InnerArchive->Position = Position;
        std::span<const uint8_t> span = InnerArchive->ReadSpan(length);
        Position = InnerArchive->Position;
        return span;
    }

    int64_t FMemoryImageArchive::BeginContainer(int32_t& arrayNum)
    {
        const int64_t initialPos = Position;
        const FFrozenMemoryImagePtr dataPtr(*this);
        arrayNum = Read<int32_t>();
        const int32_t arrayMax = Read<int32_t>();
        if (arrayNum != arrayMax)
            throw Exceptions::ParserException(*this, "Num (" + std::to_string(arrayNum) + ") != Max (" + std::to_string(arrayMax) + ")");
        if (arrayNum == 0) return Position;

        const int64_t continuePos = Position;
        Position = initialPos + dataPtr.OffsetFromThis;
        return continuePos;
    }

    std::vector<int32_t> FMemoryImageArchive::ReadHashTable()
    {
        const FFrozenMemoryImagePtr hashPtr(*this);
        const FFrozenMemoryImagePtr nextIndexPtr(*this);
        (void) hashPtr;
        (void) nextIndexPtr;
        (void) Read<uint32_t>(); // hashMask
        (void) Read<uint32_t>(); // indexSize

        return {}; // TODO always empty array for now (as in C#)
    }

    std::vector<bool> FMemoryImageArchive::ReadTBitArray()
    {
        const int64_t initialPos = Position;
        const FFrozenMemoryImagePtr dataPtr(*this);
        const int32_t numBits = Read<int32_t>();
        (void) Read<int32_t>(); // maxBits
        if (numBits == 0) return {};

        const int64_t continuePos = Position;
        Position = initialPos + dataPtr.OffsetFromThis;
        const std::vector<int32_t> data = InnerReadArray<int32_t>(Utils::DivideAndRoundUp(numBits, 32));
        Position = continuePos;

        std::vector<bool> bits(static_cast<size_t>(numBits));
        for (int32_t i = 0; i < numBits; ++i)
            bits[static_cast<size_t>(i)] = (static_cast<uint32_t>(data[static_cast<size_t>(i / 32)]) >> (i % 32) & 1u) != 0;
        return bits;
    }

    std::string FMemoryImageArchive::ReadFString()
    {
        const int64_t initialPos = Position;
        const FFrozenMemoryImagePtr dataPtr(*this);
        const int32_t arrayNum = Read<int32_t>();
        const int32_t arrayMax = Read<int32_t>();
        if (arrayNum != arrayMax)
            throw Exceptions::ParserException(*this, "Num (" + std::to_string(arrayNum) + ") != Max (" + std::to_string(arrayMax) + ")");
        if (arrayNum <= 1) return std::string();

        const int64_t continuePos = Position;
        Position = initialPos + dataPtr.OffsetFromThis;
        const std::vector<uint8_t> ucs2Bytes = ReadBytes(arrayNum * 2);
        Position = continuePos;

        if (ucs2Bytes[ucs2Bytes.size() - 1] != 0 || ucs2Bytes[ucs2Bytes.size() - 2] != 0)
            throw Exceptions::ParserException(*this, "Serialized FString is not null terminated");

        // UCS-2LE -> UTF-8. Frozen FStrings are always the wide form, so there is no ANSI branch here.
        std::string result;
        result.reserve(static_cast<size_t>(arrayNum));
        for (size_t i = 0; i + 1 < ucs2Bytes.size() - 2; i += 2)
        {
            const uint32_t c = static_cast<uint32_t>(ucs2Bytes[i]) | (static_cast<uint32_t>(ucs2Bytes[i + 1]) << 8);
            if (c < 0x80)
            {
                result.push_back(static_cast<char>(c));
            }
            else if (c < 0x800)
            {
                result.push_back(static_cast<char>(0xC0 | (c >> 6)));
                result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }
            else
            {
                result.push_back(static_cast<char>(0xE0 | (c >> 12)));
                result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }
        }
        return result;
    }

    FName FMemoryImageArchive::ReadFName()
    {
        if (Names != nullptr)
        {
            const auto it = Names->find(static_cast<int32_t>(Position));
            if (it != Names->end())
            {
                Position += it->second.second ? 12 : 8;
                return it->second.first;
            }
        }
        Position += 12;
        return FName();
    }
}
