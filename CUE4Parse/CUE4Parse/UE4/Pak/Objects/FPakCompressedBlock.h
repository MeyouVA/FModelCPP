// Ported from CUE4Parse/UE4/Pak/Objects/FPakCompressedBlock.cs
// One compressed block's [start, end) byte range inside the pak. Trivially copyable so a run of them can be
// read with a single FArchive::ReadArray.
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::UE4::Pak::Objects
{
    struct FPakCompressedBlock
    {
        int64_t CompressedStart = 0;
        int64_t CompressedEnd = 0;

        FPakCompressedBlock() = default;
        FPakCompressedBlock(int64_t compressedStart, int64_t compressedEnd)
            : CompressedStart(compressedStart), CompressedEnd(compressedEnd) {}

        int64_t Size() const { return CompressedEnd - CompressedStart; }

        std::string ToString() const
        {
            return "From " + std::to_string(CompressedStart) + " To " + std::to_string(CompressedEnd) +
                   " (=" + std::to_string(Size()) + ")";
        }
    };
}
