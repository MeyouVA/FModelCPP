// Ported from CUE4Parse/UE4/Wwise/Objects/AkStateChunk.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkSyncType.h"
#include "AkStatePropertyInfo.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkSyncType;

    struct AkStateProperty
    {
        uint16_t Id = 0;
        float Value = 0;

        AkStateProperty() = default;
        AkStateProperty(uint16_t id, float value) : Id(id), Value(value) {}
    };

    struct AkState
    {
        uint32_t Id = 0;
        std::optional<uint32_t> StateInstanceId;
        std::vector<AkStateProperty> Properties;

        AkState() = default;
        AkState(uint32_t id, std::optional<uint32_t> stateInstanceId, std::vector<AkStateProperty> properties)
            : Id(id), StateInstanceId(stateInstanceId), Properties(std::move(properties)) {}
    };

    struct AkStateGroup
    {
        uint32_t Id = 0;
        EAkSyncType GroupType = static_cast<EAkSyncType>(0);
        std::vector<AkState> States;

        AkStateGroup() = default;
        AkStateGroup(uint32_t id, EAkSyncType groupType, std::vector<AkState> states)
            : Id(id), GroupType(groupType), States(std::move(states)) {}
    };

    class AkStateChunk
    {
    public:
        std::vector<AkStateGroup> Groups;

        AkStateChunk() = default;

        explicit AkStateChunk(FWwiseArchive& Ar)
        {
            const uint32_t numGroups = Ar.Read<uint32_t>();
            Groups.reserve(numGroups);
            for (uint32_t i = 0; i < numGroups; i++)
            {
                const auto groupId = Ar.Read<uint32_t>();
                const auto stateSyncType = static_cast<EAkSyncType>(Ar.Read<uint8_t>());
                const auto numStates = Ar.Read<uint16_t>();

                std::vector<AkState> states;
                states.reserve(numStates);
                for (uint16_t s = 0; s < numStates; s++)
                {
                    const uint32_t stateId = Ar.Read<uint32_t>();
                    const uint32_t stateInstanceId = Ar.Read<uint32_t>();
                    states.emplace_back(stateId, stateInstanceId, std::vector<AkStateProperty>{});
                }

                Groups.emplace_back(groupId, stateSyncType, std::move(states));
            }
        }
    };

    class AkStateAwareChunk
    {
    public:
        std::vector<AkStatePropertyInfo> StateProperties;
        std::vector<AkStateGroup> Groups;

        AkStateAwareChunk() = default;

        // CAkStateAware::ReadStateChunk
        // Note the counts here are 7-bit-encoded big-endian, not the plain uint/ushort AkStateChunk uses.
        explicit AkStateAwareChunk(FWwiseArchive& Ar)
        {
            const int propInfoCount = Ar.Read7BitEncodedIntBE();
            StateProperties = Ar.ReadArrayWith(propInfoCount, [&Ar] { return AkStatePropertyInfo(Ar); });

            const int groupCount = Ar.Read7BitEncodedIntBE();
            Groups.reserve(static_cast<size_t>(groupCount));
            for (int g = 0; g < groupCount; g++)
            {
                const uint32_t groupId = Ar.Read<uint32_t>();

                if (Ar.Version > 154)
                {
                    Ar.Read<uint32_t>(); // groupUsageId -- read and dropped, as in C#
                }

                const auto stateSyncType = static_cast<EAkSyncType>(Ar.Read<uint8_t>());

                const int stateCount = Ar.Read7BitEncodedIntBE();
                std::vector<AkState> states;
                states.reserve(static_cast<size_t>(stateCount));
                for (int s = 0; s < stateCount; s++)
                {
                    const uint32_t stateId = Ar.Read<uint32_t>();
                    if (Ar.Version <= 145)
                    {
                        const uint32_t stateInstanceId = Ar.Read<uint32_t>();
                        states.emplace_back(stateId, stateInstanceId, std::vector<AkStateProperty>{});
                    }
                    else
                    {
                        const uint16_t propCount = Ar.Read<uint16_t>();
                        std::vector<AkStateProperty> props;
                        props.reserve(propCount);
                        for (uint16_t k = 0; k < propCount; k++)
                        {
                            const uint16_t propId = Ar.Read<uint16_t>();
                            const float value = Ar.Read<float>();
                            props.emplace_back(propId, value);
                        }
                        states.emplace_back(stateId, std::nullopt, std::move(props));
                    }
                }

                Groups.emplace_back(groupId, stateSyncType, std::move(states));
            }
        }
    };
}
