// Ported from CUE4Parse/UE4/Versions/ObjectVersion.cs (FPackageFileVersion struct)
// Holds the UE3/UE4/UE5 file versions of a package and the comparison helpers used all over the reader.
#pragma once

#include <cstdint>
#include "ObjectVersion.h"

namespace CUE4Parse::UE4::Versions
{
    struct FPackageFileVersion
    {
        int32_t FileVersionUE3 = 0;
        int32_t FileVersionUE4 = 0;
        int32_t FileVersionUE5 = 0;

        FPackageFileVersion() = default;

        FPackageFileVersion(int32_t ue4Version, int32_t ue5Version)
            : FileVersionUE3(static_cast<int32_t>(EUnrealEngineObjectUE3Version::LegacyUE3Version))
            , FileVersionUE4(ue4Version)
            , FileVersionUE5(ue5Version) {}

        FPackageFileVersion(int32_t ue3Version, int32_t ue4Version, int32_t ue5Version)
            : FileVersionUE3(ue3Version), FileVersionUE4(ue4Version), FileVersionUE5(ue5Version) {}

        void Reset() { FileVersionUE3 = 0; FileVersionUE4 = 0; FileVersionUE5 = 0; }

        // Creates a FPackageFileVersion based on a single UE3/UE4/UE5 version.
        static FPackageFileVersion CreateUE3Version(int32_t version) { return {version, 0, 0}; }
        static FPackageFileVersion CreateUE3Version(EUnrealEngineObjectUE3Version version) { return {static_cast<int32_t>(version), 0, 0}; }
        static FPackageFileVersion CreateUE4Version(int32_t version) { return {version, 0}; }
        static FPackageFileVersion CreateUE4Version(EUnrealEngineObjectUE4Version version) { return {static_cast<int32_t>(version), 0}; }

        int32_t GetValue() const
        {
            if (FileVersionUE5 >= static_cast<int32_t>(EUnrealEngineObjectUE5Version::INITIAL_VERSION))
                return FileVersionUE5;
            if (FileVersionUE4 > static_cast<int32_t>(EUnrealEngineObjectUE4Version::DETERMINE_BY_GAME))
                return FileVersionUE4;
            return FileVersionUE3;
        }

        void SetValue(int32_t value)
        {
            if (value >= static_cast<int32_t>(EUnrealEngineObjectUE5Version::INITIAL_VERSION))
                FileVersionUE5 = value;
            else if (value > static_cast<int32_t>(EUnrealEngineObjectUE4Version::DETERMINE_BY_GAME))
                FileVersionUE4 = value;
            else
                FileVersionUE3 = value;
        }

        // Returns true if all version numbers here are >= the corresponding numbers of other.
        bool IsCompatible(const FPackageFileVersion& other) const
        {
            return FileVersionUE3 >= other.FileVersionUE3 && FileVersionUE4 >= other.FileVersionUE4 && FileVersionUE5 >= other.FileVersionUE5;
        }
    };

    // UE3 version comparisons
    inline bool operator==(FPackageFileVersion a, EUnrealEngineObjectUE3Version b) { return a.FileVersionUE3 == static_cast<int32_t>(b); }
    inline bool operator!=(FPackageFileVersion a, EUnrealEngineObjectUE3Version b) { return a.FileVersionUE3 != static_cast<int32_t>(b); }
    inline bool operator< (FPackageFileVersion a, EUnrealEngineObjectUE3Version b) { return a.FileVersionUE3 <  static_cast<int32_t>(b); }
    inline bool operator> (FPackageFileVersion a, EUnrealEngineObjectUE3Version b) { return a.FileVersionUE3 >  static_cast<int32_t>(b); }
    inline bool operator<=(FPackageFileVersion a, EUnrealEngineObjectUE3Version b) { return a.FileVersionUE3 <= static_cast<int32_t>(b); }
    inline bool operator>=(FPackageFileVersion a, EUnrealEngineObjectUE3Version b) { return a.FileVersionUE3 >= static_cast<int32_t>(b); }

    // UE4 version comparisons
    inline bool operator==(FPackageFileVersion a, EUnrealEngineObjectUE4Version b) { return a.FileVersionUE4 == static_cast<int32_t>(b); }
    inline bool operator!=(FPackageFileVersion a, EUnrealEngineObjectUE4Version b) { return a.FileVersionUE4 != static_cast<int32_t>(b); }
    inline bool operator< (FPackageFileVersion a, EUnrealEngineObjectUE4Version b) { return a.FileVersionUE4 <  static_cast<int32_t>(b); }
    inline bool operator> (FPackageFileVersion a, EUnrealEngineObjectUE4Version b) { return a.FileVersionUE4 >  static_cast<int32_t>(b); }
    inline bool operator<=(FPackageFileVersion a, EUnrealEngineObjectUE4Version b) { return a.FileVersionUE4 <= static_cast<int32_t>(b); }
    inline bool operator>=(FPackageFileVersion a, EUnrealEngineObjectUE4Version b) { return a.FileVersionUE4 >= static_cast<int32_t>(b); }

    // UE5 version comparisons
    inline bool operator==(FPackageFileVersion a, EUnrealEngineObjectUE5Version b) { return a.FileVersionUE5 == static_cast<int32_t>(b); }
    inline bool operator!=(FPackageFileVersion a, EUnrealEngineObjectUE5Version b) { return a.FileVersionUE5 != static_cast<int32_t>(b); }
    inline bool operator< (FPackageFileVersion a, EUnrealEngineObjectUE5Version b) { return a.FileVersionUE5 <  static_cast<int32_t>(b); }
    inline bool operator> (FPackageFileVersion a, EUnrealEngineObjectUE5Version b) { return a.FileVersionUE5 >  static_cast<int32_t>(b); }
    inline bool operator<=(FPackageFileVersion a, EUnrealEngineObjectUE5Version b) { return a.FileVersionUE5 <= static_cast<int32_t>(b); }
    inline bool operator>=(FPackageFileVersion a, EUnrealEngineObjectUE5Version b) { return a.FileVersionUE5 >= static_cast<int32_t>(b); }

    // FPackageFileVersion comparisons
    inline bool operator==(FPackageFileVersion a, FPackageFileVersion b)
    {
        return a.FileVersionUE3 == b.FileVersionUE3 && a.FileVersionUE4 == b.FileVersionUE4 && a.FileVersionUE5 == b.FileVersionUE5;
    }
    inline bool operator!=(FPackageFileVersion a, FPackageFileVersion b) { return !(a == b); }
}
