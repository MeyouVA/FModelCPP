// Ported from CUE4Parse/UE4/FMod/Objects/FEventParameterStub.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FEventParameterStub
    {
        uint32_t StubIndex = 0;
        FModGuid ParameterGuid;
        float InitialValue = 0.0f;

        FEventParameterStub() = default;
        explicit FEventParameterStub(Readers::FArchive& Ar)
        {
            StubIndex = Ar.Read<uint32_t>();
            ParameterGuid = FModGuid(Ar);
            InitialValue = Ar.Read<float>();
        }
    };
}
