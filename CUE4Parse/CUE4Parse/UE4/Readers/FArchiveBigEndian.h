// Ported from CUE4Parse/UE4/Readers/FArchiveBigEndian.cs
// Wraps another archive and byte-swaps multi-byte values on read (for big-endian cooked data).
//
// IMPORTANT difference from C#: the C# version uses runtime reflection (Marshal.OffsetOf, field
// walking) to discover a struct's field layout and swap each field of an arbitrary `Read<T>()`.
// C++ has no such reflection. Here, endian swapping is applied per *scalar* via the ReadScalar /
// ReadElements seams in FArchive, which means:
//   * Read<Scalar>() and ReadArray<Scalar>() (integers, floats, enums) swap correctly.
//   * Composite structs must be read field-by-field through the archive (each field goes through a
//     scalar read and is swapped). A single bulk Read<MyStruct>() will NOT swap interior fields.
// This matches the idiomatic C++ "each struct reads itself from the archive" pattern. TODO: revisit
// if a caller genuinely needs bulk struct swaps.
//
// The wrapped archive is referenced, not owned; it must outlive this wrapper.
#pragma once

#include <string>

#include "FArchive.h"

namespace CUE4Parse::UE4::Readers
{
    class FArchiveBigEndian : public FArchive
    {
    public:
        explicit FArchiveBigEndian(FArchive* baseArchive);

        using FArchive::Read; // keep the inherited Read<T> template visible alongside the byte-source override

        int Read(uint8_t* buffer, int offset, int count) override;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        bool CanSeek() const override;
        const std::string& Name() const override;
        std::unique_ptr<FArchive> Clone() const override;

        void Serialize(uint8_t* ptr, int length) override;
        void ReadScalar(void* dst, int size) override;
        void ReadElements(void* dst, int elementSize, int count) override;

        // Big-endian FString override: a UTF-8, count-prefixed byte blob (count itself is big-endian).
        std::string ReadString();

    private:
        FArchive* _baseArchive;
    };
}
