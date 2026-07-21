// Ported from CUE4Parse/UE4/Objects/UObject/CoreNetTypes.cs (ELifetimeCondition only, for FProperty).
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::UObject
{
    enum class ELifetimeCondition : uint8_t
    {
        COND_None = 0,                      // No condition; sends anytime it changes.
        COND_InitialOnly = 1,               // Only on the initial bunch.
        COND_OwnerOnly = 2,                 // Only to the actor's owner.
        COND_SkipOwner = 3,                 // To every connection EXCEPT the owner.
        COND_SimulatedOnly = 4,             // Only to simulated actors.
        COND_AutonomousOnly = 5,            // Only to autonomous actors.
        COND_SimulatedOrPhysics = 6,        // To simulated OR bRepPhysics actors.
        COND_InitialOrOwner = 7,            // On the initial packet, or to the actor's owner.
        COND_Custom = 8,                    // Toggle on/off via SetCustomIsActiveOverride.
        COND_ReplayOrOwner = 9,             // Only to the replay connection, or to the actor's owner.
        COND_ReplayOnly = 10,               // Only to the replay connection.
        COND_SimulatedOnlyNoReplay = 11,    // To actors only, but not replay connections.
        COND_SimulatedOrPhysicsNoReplay = 12,
        COND_SkipReplay = 13,               // Not to the replay connection.
        COND_Max = 14,
    };
}
