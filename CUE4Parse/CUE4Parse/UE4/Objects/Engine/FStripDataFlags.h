// Ported from CUE4Parse/UE4/Objects/Engine/FStripDataFlags.cs
// Two bytes an editor-aware package writes ahead of data that may have been stripped on cook. Read purely
// for its side effect on the cursor in most call sites (C# writes `_ = new FStripDataFlags(Ar);`).
#pragma once

#include <cstdint>

#include "../../Readers/FArchive.h"
#include "../../Versions/FPackageFileVersion.h"
#include "../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Objects::Engine
{
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;
    using CUE4Parse::UE4::Versions::FPackageFileVersion;

    struct FStripDataFlags
    {
        uint8_t GlobalStripFlags = 0;
        uint8_t ClassStripFlags = 0;

        // C#'s static OldestLoadablePackageFileUEVersion, spelled as a function so it has no static-init order.
        static FPackageFileVersion OldestLoadablePackageFileUEVersion()
        {
            return FPackageFileVersion::CreateUE4Version(EUnrealEngineObjectUE4Version::REMOVED_STRIP_DATA);
        }

        explicit FStripDataFlags(Readers::FArchive& Ar)
            : FStripDataFlags(Ar, OldestLoadablePackageFileUEVersion()) {}

        FStripDataFlags(Readers::FArchive& Ar, const FPackageFileVersion& minVersion)
        {
            if (Ar.Ver().IsCompatible(minVersion))
            {
                GlobalStripFlags = Ar.Read<uint8_t>();
                ClassStripFlags = Ar.Read<uint8_t>();
            }
        }

        bool IsEditorDataStripped() const { return (GlobalStripFlags & 1) != 0; }
        bool IsAudioVisualDataStripped() const { return (GlobalStripFlags & 2) != 0; }
        bool IsDataNeededForCookingStripped() const { return (GlobalStripFlags & 4) != 0; }
        bool IsClassDataStripped(uint8_t flag) const { return (ClassStripFlags & flag) != 0; }
    };
}
