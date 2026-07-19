// Ported from CUE4Parse/UE4/Readers/FArchiveBigEndian.cs
#include "FArchiveBigEndian.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace CUE4Parse::UE4::Readers
{
    namespace
    {
        inline uint16_t Bswap16(uint16_t v) { return static_cast<uint16_t>((v << 8) | (v >> 8)); }
        inline uint32_t Bswap32(uint32_t v)
        {
            return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
                   ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
        }
        inline uint64_t Bswap64(uint64_t v)
        {
            v = ((v & 0x00000000FFFFFFFFull) << 32) | ((v & 0xFFFFFFFF00000000ull) >> 32);
            v = ((v & 0x0000FFFF0000FFFFull) << 16) | ((v & 0xFFFF0000FFFF0000ull) >> 16);
            v = ((v & 0x00FF00FF00FF00FFull) << 8)  | ((v & 0xFF00FF00FF00FF00ull) >> 8);
            return v;
        }

        // Reverses the endianness of `count` elements of `elementSize` bytes in place.
        void ReverseEndian(void* data, int elementSize, int count)
        {
            auto* p = static_cast<uint8_t*>(data);
            switch (elementSize)
            {
                case 1:
                    return;
                case 2:
                    for (int i = 0; i < count; i++)
                    {
                        uint16_t v; std::memcpy(&v, p + i * 2, 2); v = Bswap16(v); std::memcpy(p + i * 2, &v, 2);
                    }
                    break;
                case 4:
                    for (int i = 0; i < count; i++)
                    {
                        uint32_t v; std::memcpy(&v, p + i * 4, 4); v = Bswap32(v); std::memcpy(p + i * 4, &v, 4);
                    }
                    break;
                case 8:
                    for (int i = 0; i < count; i++)
                    {
                        uint64_t v; std::memcpy(&v, p + i * 8, 8); v = Bswap64(v); std::memcpy(p + i * 8, &v, 8);
                    }
                    break;
                default:
                    throw std::runtime_error("Unsupported size " + std::to_string(elementSize) + " for reversing endianness");
            }
        }
    }

    FArchiveBigEndian::FArchiveBigEndian(FArchive* baseArchive)
        : FArchive(baseArchive->Versions), _baseArchive(baseArchive)
    {
        Length = _baseArchive->Length;
        Position = _baseArchive->Position;
    }

    int FArchiveBigEndian::Read(uint8_t* buffer, int offset, int count)
    {
        _baseArchive->Position = Position;
        const int n = _baseArchive->Read(buffer, offset, count);
        Position += n;
        return n;
    }

    void FArchiveBigEndian::Serialize(uint8_t* ptr, int length)
    {
        _baseArchive->Position = Position;
        _baseArchive->Serialize(ptr, length);
        Position += length;
    }

    void FArchiveBigEndian::ReadScalar(void* dst, int size)
    {
        Serialize(static_cast<uint8_t*>(dst), size);
        ReverseEndian(dst, size, 1);
    }

    void FArchiveBigEndian::ReadElements(void* dst, int elementSize, int count)
    {
        if (count <= 0) return;
        Serialize(static_cast<uint8_t*>(dst), elementSize * count);
        ReverseEndian(dst, elementSize, count);
    }

    std::string FArchiveBigEndian::ReadString()
    {
        auto bytes = ReadArrayCounted<uint8_t>(); // count is read big-endian via ReadScalar
        return std::string(bytes.begin(), bytes.end());
    }

    int64_t FArchiveBigEndian::Seek(int64_t offset, ESeekOrigin origin)
    {
        Position = _baseArchive->Seek(offset, origin);
        return Position;
    }

    bool FArchiveBigEndian::CanSeek() const { return _baseArchive->CanSeek(); }
    const std::string& FArchiveBigEndian::Name() const { return _baseArchive->Name(); }

    std::unique_ptr<FArchive> FArchiveBigEndian::Clone() const
    {
        auto clone = std::make_unique<FArchiveBigEndian>(_baseArchive);
        clone->Position = Position;
        return clone;
    }
}
