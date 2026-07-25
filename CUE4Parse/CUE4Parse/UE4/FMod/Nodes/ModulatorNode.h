// Ported from CUE4Parse/UE4/FMod/Nodes/ModulatorNode.cs
// C#'s `object? Subnode` becomes a std::variant of the six concrete modulator subnode types.
#pragma once

#include <variant>

#include "../Objects/FModGuid.h"
#include "../Enums/EModulatorType.h"
#include "../Enums/EPropertyType.h"
#include "../Enums/EClockSource.h"
#include "ModulatorSubnodes/ADSRModulatorNode.h"
#include "ModulatorSubnodes/RandomModulatorNode.h"
#include "ModulatorSubnodes/EnvelopeModulatorNode.h"
#include "ModulatorSubnodes/LFOModulatorNode.h"
#include "ModulatorSubnodes/SeekModulatorNode.h"
#include "ModulatorSubnodes/SpectralSidechainModulatorNode.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class ModulatorNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid OwnerGuid;
        int32_t PropertyIndex = 0;
        Enums::EModulatorType Type{};
        Enums::EPropertyType PropertyType{};
        Enums::EClockSource ClockSource{};
        std::variant<std::monostate,
                     ModulatorSubnodes::ADSRModulatorNode,
                     ModulatorSubnodes::RandomModulatorNode,
                     ModulatorSubnodes::EnvelopeModulatorNode,
                     ModulatorSubnodes::LFOModulatorNode,
                     ModulatorSubnodes::SeekModulatorNode,
                     ModulatorSubnodes::SpectralSidechainModulatorNode> Subnode;

        explicit ModulatorNode(Readers::FArchive& Ar)
        {
            if (FModReader::Version() >= 0x55) (void) Ar.Read<uint16_t>(); // Payload size

            BaseGuid = Objects::FModGuid(Ar);
            OwnerGuid = Objects::FModGuid(Ar);

            PropertyIndex = Ar.Read<int32_t>();
            Type = static_cast<Enums::EModulatorType>(Ar.Read<int32_t>());

            if (FModReader::Version() < 0x55)
            {
                (void) Ar.Read<int16_t>(); // Payload size
            }
            else
            {
                PropertyType = static_cast<Enums::EPropertyType>(Ar.Read<int32_t>());
                ClockSource = FModReader::Version() >= 0x90
                    ? static_cast<Enums::EClockSource>(Ar.Read<int32_t>())
                    : Enums::EClockSource::ClockSource_Local;
            }

            switch (Type)
            {
                case Enums::EModulatorType::ADSR:
                    Subnode.emplace<ModulatorSubnodes::ADSRModulatorNode>(Ar);
                    break;
                case Enums::EModulatorType::Random:
                    Subnode.emplace<ModulatorSubnodes::RandomModulatorNode>(Ar);
                    break;
                case Enums::EModulatorType::Envelope:
                    Subnode.emplace<ModulatorSubnodes::EnvelopeModulatorNode>(Ar);
                    break;
                case Enums::EModulatorType::LFO:
                    Subnode.emplace<ModulatorSubnodes::LFOModulatorNode>(Ar);
                    break;
                case Enums::EModulatorType::Seek:
                    Subnode.emplace<ModulatorSubnodes::SeekModulatorNode>(Ar);
                    break;
                case Enums::EModulatorType::SpectralSidechain:
                    Subnode.emplace<ModulatorSubnodes::SpectralSidechainModulatorNode>(Ar);
                    break;
                default:
                    // C# logs an error for an unhandled modulator type; no logging layer in the port.
                    break;
            }
        }
    };
}
