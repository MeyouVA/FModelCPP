// Ported from CUE4Parse/UE4/FMod/Nodes/ParameterNode.cs
#pragma once

#include <string>
#include <vector>

#include "../Objects/FModGuid.h"
#include "../Enums/EFModStudioParameterType.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class ParameterNode
    {
    public:
        Objects::FModGuid BaseGuid;
        int32_t Flags = 0;
        Enums::EFModStudioParameterType Type{};
        std::string Name;
        float Minimum = 0.0f;
        float Maximum = 0.0f;
        float DefaultValue = 0.0f;
        float Velocity = 0.0f;
        float SeekSpeed = 0.0f;
        float SeekSpeedDown = 0.0f;
        std::vector<std::string> Labels;

        explicit ParameterNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            if (FModReader::Version() >= 0x70) Flags = Ar.Read<int32_t>();
            if (FModReader::Version() < 0x70) (void) Ar.Read<uint8_t>();

            Type = static_cast<Enums::EFModStudioParameterType>(Ar.Read<uint32_t>());
            Name = FModReader::ReadString(Ar);
            Minimum = Ar.Read<float>();
            Maximum = Ar.Read<float>();
            DefaultValue = Ar.Read<float>();
            Velocity = Ar.Read<float>();

            if (FModReader::Version() < 0x8f) SeekSpeed = Ar.Read<float>();
            if (FModReader::Version() < 0x70) (void) Ar.Read<uint8_t>();
            if (FModReader::Version() < 0x60) (void) FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            if (FModReader::Version() > 0x52 && FModReader::Version() <= 0x8E) SeekSpeedDown = Ar.Read<float>();
            if (FModReader::Version() > 0x52 && FModReader::Version() <= 0x6F) (void) Ar.Read<uint8_t>();

            Labels = FModReader::Version() >= 0x8b
                ? FModReader::ReadVersionedElemListImp(Ar, [](Readers::FArchive& a) { return FModReader::ReadString(a); })
                : std::vector<std::string>{};
        }
    };
}
