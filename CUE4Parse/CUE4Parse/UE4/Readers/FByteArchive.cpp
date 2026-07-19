// Ported from CUE4Parse/UE4/Readers/FByteArchive.cs
#include "FByteArchive.h"

#include <stdexcept>

namespace CUE4Parse::UE4::Readers
{
    int FByteArchive::Read(uint8_t* buffer, int offset, int count)
    {
        int n = static_cast<int>(Length - Position);
        if (n > count) n = count;
        if (n <= 0) return 0;

        if (n <= 8)
        {
            int byteCount = n;
            while (--byteCount >= 0)
                buffer[offset + byteCount] = _data[Position + byteCount];
        }
        else
        {
            std::memcpy(buffer + offset, _data.data() + Position, static_cast<size_t>(n));
        }
        Position += n;
        return n;
    }

    int FByteArchive::ReadAt(int64_t position, uint8_t* buffer, int offset, int count)
    {
        int n = static_cast<int>(Length - position);
        if (n > count) n = count;
        if (n <= 0) return 0;

        if (n <= 8)
        {
            int byteCount = n;
            while (--byteCount >= 0)
                buffer[offset + byteCount] = _data[position + byteCount];
        }
        else
        {
            std::memcpy(buffer + offset, _data.data() + position, static_cast<size_t>(n));
        }
        return n;
    }

    int64_t FByteArchive::Seek(int64_t offset, ESeekOrigin origin)
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

    std::span<const uint8_t> FByteArchive::ReadSpan(int length)
    {
        CheckReadSize(length);
        std::span<const uint8_t> result(_data.data() + Position, static_cast<size_t>(length));
        Position += length;
        return result;
    }

    void FByteArchive::Serialize(uint8_t* ptr, int length)
    {
        std::memcpy(ptr, _data.data() + Position, static_cast<size_t>(length));
        Position += length;
    }

    std::unique_ptr<FArchive> FByteArchive::Clone() const
    {
        auto clone = std::make_unique<FByteArchive>(_name, _data, Versions);
        clone->Position = Position;
        return clone;
    }
}
