// Ported from CUE4Parse/UE4/Objects/Core/Misc/FEngineVersionBase.cs
#pragma once

#include <cstdint>

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Misc
{
    // Enum for the components of a version string.
    enum class EVersionComponent
    {
        Major,      // Major version increments introduce breaking API changes.
        Minor,      // Minor version increments add functionality without breaking existing APIs.
        Patch,      // Patch version increments fix existing functionality without changing the API.
        Changelist, // Additional versioning through a series of comparable dotted strings or numbers.
        Branch
    };

    class FEngineVersionBase
    {
    public:
        uint16_t Major = 0; // Major version number.
        uint16_t Minor = 0; // Minor version number.
        uint16_t Patch = 0; // Patch version number.

        // Changelist number, used to arbitrate when Major/Minor/Patch match. Masks off the licensee bit.
        uint32_t Changelist() const { return _changelist & 0x7fffffffu; }

        explicit FEngineVersionBase(Readers::FArchive& Ar)
        {
            Major = Ar.Read<uint16_t>();
            Minor = Ar.Read<uint16_t>();
            Patch = Ar.Read<uint16_t>();
            _changelist = Ar.Read<uint32_t>();
        }

        FEngineVersionBase(uint16_t major, uint16_t minor, uint16_t patch, uint32_t changelist)
            : Major(major), Minor(minor), Patch(patch), _changelist(changelist) {}

        virtual ~FEngineVersionBase() = default;

        // Checks if the changelist number represents a licensee changelist number.
        bool IsLicenseeVersion() const { return (_changelist & 0x80000000u) != 0u; }

        // Returns whether the current version is empty.
        bool IsEmpty() const { return Major == 0u && Minor == 0u && Patch == 0u; }

        // Returns whether the engine version has a changelist component.
        bool HasChangelist() const { return Changelist() != 0; }

        // Encodes a licensee changelist number (by setting the top bit).
        static uint32_t EncodeLicenseeChangeList(uint32_t changelist) { return changelist | 0x80000000u; }

    protected:
        uint32_t _changelist = 0;
    };
}
