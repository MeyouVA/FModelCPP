// Ported from CUE4Parse/UE4/Wwise/Objects/AkEntry.cs
// One record in an .pck (AKPK) index: a hashed name, a folder id, and where in the package the payload is.
// The offset is stored divided by OffsetMultiplier, so the real byte offset is the product of the two.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../WwiseArchive.h"
#include "../FDeferredByteData.h"
#include "AkFolder.h"

namespace CUE4Parse::UE4::Wwise
{
    struct WwiseDataSource;
}

namespace CUE4Parse::UE4::Wwise::Objects
{
    class AkEntry
    {
    public:
        uint32_t NameHash = 0;
        uint64_t ExternalNameHash = 0;
        uint32_t OffsetMultiplier = 0;
        int32_t Size = 0;
        uint32_t Offset = 0;
        uint32_t FolderId = 0;
        std::string AudioPath;
        bool IsSoundBank = false;
        bool IsExternalSound = false;
        std::shared_ptr<FDeferredByteData> Data;

        AkEntry(FWwiseArchive& Ar, bool isSoundBank, bool externalSound = false)
            : IsSoundBank(isSoundBank), IsExternalSound(externalSound)
        {
            // The two name kinds are different widths, so this is not just a cast of the same field.
            if (externalSound) ExternalNameHash = Ar.Read<uint64_t>();
            else               NameHash = Ar.Read<uint32_t>();

            OffsetMultiplier = Ar.Read<uint32_t>();
            Size = Ar.Read<int32_t>();
            Offset = Ar.Read<uint32_t>();
            FolderId = Ar.Read<uint32_t>();
        }

        std::string Name() const
        {
            return IsExternalSound ? std::to_string(ExternalNameHash) : std::to_string(NameHash);
        }

        void ReadAudioPath(const std::vector<AkFolder>& folders)
        {
            std::string folderName;
            for (const AkFolder& folder : folders)
            {
                // AkFolder::Name is optional (C#'s `string?`); C# coalesces a miss to "".
                if (folder.Id == FolderId) { folderName = folder.Name.value_or(std::string()); break; }
            }
            // Path.Combine: an empty first part yields just the file name, and a separator is inserted
            // only when the folder does not already end in one.
            const std::string fileName = Name() + (IsSoundBank ? ".bnk" : ".wem");
            if (folderName.empty()) { AudioPath = fileName; return; }
            const char last = folderName.back();
            AudioPath = folderName + ((last == '/' || last == '\\') ? "" : "/") + fileName;
        }

        // Defined in WwiseReader.h, which owns ReadDeferredByteData (and knows WwiseDataSource).
        int64_t ReadData(FWwiseArchive& Ar, const WwiseDataSource& source);
    };
}
