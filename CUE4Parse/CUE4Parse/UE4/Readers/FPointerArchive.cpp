// Ported from CUE4Parse/UE4/Readers/FPointerArchive.cs
#include "FPointerArchive.h"

#include <stdexcept>

namespace CUE4Parse::UE4::Readers
{
    int FPointerArchive::Read(uint8_t* buffer, int offset, int count)
    {
        int n = static_cast<int>(Length - Position);
        if (n > count) n = count;
        if (n <= 0) return 0;

        if (n <= 8)
        {
            int byteCount = n;
            while (--byteCount >= 0)
                buffer[offset + byteCount] = _ptr[Position + byteCount];
        }
        else
        {
            std::memcpy(buffer + offset, _ptr + Position, static_cast<size_t>(n));
        }
        Position += n;
        return n;
    }

    int FPointerArchive::ReadAt(int64_t position, uint8_t* buffer, int offset, int count)
    {
        int n = static_cast<int>(Length - position);
        if (n > count) n = count;
        if (n <= 0) return 0;

        if (n <= 8)
        {
            int byteCount = n;
            while (--byteCount >= 0)
                buffer[offset + byteCount] = _ptr[position + byteCount];
        }
        else
        {
            std::memcpy(buffer + offset, _ptr + position, static_cast<size_t>(n));
        }
        return n;
    }

    int64_t FPointerArchive::Seek(int64_t offset, ESeekOrigin origin)
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

    std::vector<uint8_t> FPointerArchive::ReadBytes(int length)
    {
        CheckReadSize(length);
        std::vector<uint8_t> buffer(static_cast<size_t>(length));
        if (length > 0) Read(buffer.data(), 0, length);
        return buffer;
    }

    void FPointerArchive::Serialize(uint8_t* ptr, int length)
    {
        std::memcpy(ptr, _ptr + Position, static_cast<size_t>(length));
        Position += length;
    }

    std::unique_ptr<FArchive> FPointerArchive::Clone() const
    {
        auto clone = std::make_unique<FPointerArchive>(_name, _ptr, Length, Versions);
        clone->Position = Position;
        return clone;
    }
}
