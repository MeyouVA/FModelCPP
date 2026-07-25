// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyEvent.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    // CAkEvent
    class HierarchyEvent : public AbstractHierarchy
    {
    public:
        uint8_t LimitScope = 0;
        uint16_t InstanceLimit = 0;
        float CooldownTime = 0;
        std::vector<uint32_t> EventActionIds;

        // CAkEvent::SetInitialValues
        explicit HierarchyEvent(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            if (Ar.Version > 154)
            {
                LimitScope = Ar.Read<uint8_t>();
                InstanceLimit = Ar.Read<uint16_t>();
                CooldownTime = Ar.Read<float>();
            }

            // The count switched from a plain uint to a 7-bit-encoded BE int at 123.
            int eventActionCount;
            if (Ar.Version <= 122)
                eventActionCount = static_cast<int>(Ar.Read<uint32_t>());
            else
                eventActionCount = Ar.Read7BitEncodedIntBE();

            EventActionIds = Ar.ReadArray<uint32_t>(eventActionCount);
        }
    };
}
