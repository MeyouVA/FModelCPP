// Ported from CUE4Parse/UE4/IO/Objects/FIoContainerHeaderSerialInfo.cs
#pragma once

#include <cstdint>

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::IO::Objects
{
    struct FIoContainerHeaderSerialInfo
    {
        int64_t Offset = 0;
        int64_t Size = 0;

        FIoContainerHeaderSerialInfo() = default;

        explicit FIoContainerHeaderSerialInfo(Readers::FArchive& Ar)
        {
            Offset = Ar.Read<int64_t>();
            Size = Ar.Read<int64_t>();
        }
    };
}
