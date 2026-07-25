// Ported from CUE4Parse/UE4/Wwise/WwiseReader.cs
// The chunk walker for a Wwise container: an .bnk soundbank, an .pck package (AKPK) or a bare .wem. Reads
// the file as a flat sequence of (4-byte tag, 4-byte length, payload) sections and dispatches on the tag,
// always re-seeking to the declared end of each section afterwards.
//
// Deliberate differences from C#:
//   * C#'s `abstract record WwiseDataSource` + three case records become one small struct with a Kind tag.
//     The payloads (GameFile, FAssetArchive + FByteBulkData) are non-owning references, exactly as the C#
//     records hold references.
//   * No logging: C# warns on an unsupported version, an unknown section tag, and a section that read the
//     wrong number of bytes. The corrective seek those warnings accompany is kept.
//   * The nested banks of an AKPK are held by unique_ptr (C# has a List of references).
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "WwiseArchive.h"
#include "FDeferredByteData.h"
#include "Enums/EChunkID.h"
#include "Objects/AkBankHeader.h"
#include "Objects/AkEntry.h"
#include "Objects/AkFolder.h"
#include "Objects/CAkEnvironmentsMgr.h"
#include "Objects/GlobalSettings.h"
#include "Objects/MediaHeader.h"
#include "Objects/HIRC/Hierarchy.h"
#include "../Assets/Objects/FByteBulkData.h"
#include "../Assets/Readers/FAssetArchive.h"
#include "../Exceptions/ParserException.h"
#include "../../FileProvider/Objects/GameFile.h"

namespace CUE4Parse::UE4::Wwise
{
    using CUE4Parse::UE4::Wwise::Enums::EChunkID;
    using CUE4Parse::UE4::Wwise::Objects::AkBankHeader;
    using CUE4Parse::UE4::Wwise::Objects::AkEntry;
    using CUE4Parse::UE4::Wwise::Objects::AkFolder;
    using CUE4Parse::UE4::Wwise::Objects::CAkEnvironmentsMgr;
    using CUE4Parse::UE4::Wwise::Objects::FAKPKHeader;
    using CUE4Parse::UE4::Wwise::Objects::GlobalSettings;
    using CUE4Parse::UE4::Wwise::Objects::MediaHeader;
    using CUE4Parse::UE4::Wwise::Objects::HIRC::Hierarchy;

    // A RIFF section that claims to run past the end of the archive. C# throws this to let the caller
    // distinguish "this isn't a wem after all" from a real parse failure.
    class RIFFSectionSizeException : public std::runtime_error
    {
    public:
        RIFFSectionSizeException() : std::runtime_error("RIFF section is larger than the remaining archive") {}
    };

    // C#'s `abstract record WwiseDataSource` and its three cases, folded into a tagged struct.
    struct WwiseDataSource
    {
        enum class EKind { Archive, GameFile, BulkData };

        EKind Kind = EKind::Archive;
        FileProvider::Objects::GameFile* File = nullptr;                 // GameFile case
        Assets::Readers::FAssetArchive* AssetAr = nullptr;               // BulkData case
        const Assets::Objects::FByteBulkData* BulkData = nullptr;        // BulkData case

        static WwiseDataSource Archive() { return WwiseDataSource{}; }
        static WwiseDataSource FromGameFile(FileProvider::Objects::GameFile& file)
        {
            WwiseDataSource s; s.Kind = EKind::GameFile; s.File = &file; return s;
        }
        static WwiseDataSource FromBulkData(Assets::Readers::FAssetArchive& assetAr,
                                            const Assets::Objects::FByteBulkData& bulkData)
        {
            WwiseDataSource s; s.Kind = EKind::BulkData; s.AssetAr = &assetAr; s.BulkData = &bulkData; return s;
        }
    };

    class WwiseReader
    {
    public:
        std::string Path;

        AkBankHeader Header;
        std::vector<std::unique_ptr<WwiseReader>> AKPKBankEntries;
        std::vector<AkEntry> AKPKWemEntries;
        std::unordered_map<uint32_t, std::string> AKPluginList;
        std::vector<MediaHeader> WemIndexes;
        std::vector<Hierarchy> Hierarchies;
        std::unordered_map<uint32_t, std::string> BankIDToFileName;
        std::string Platform;
        // Ordered so an extraction walk is reproducible; C#'s Dictionary order is incidental.
        std::map<std::string, std::shared_ptr<FDeferredByteData>> WwiseEncodedMedias;
        std::optional<GlobalSettings> GlobalSettings_;
        std::optional<CAkEnvironmentsMgr> EnvSettings;
        std::shared_ptr<FDeferredByteData> WemFile;
        std::shared_ptr<FDeferredByteData> MidiData;
        bool IsPlugin = false;
        int64_t LoadedSize = 0;
        int64_t TotalSize = 0;

        WwiseReader(FWwiseArchive& Ar, const WwiseDataSource& source, int64_t size = -1);

        // Records where `size` bytes live rather than reading them, when the source supports partial reads.
        // The archive is always left just past the range either way, so the section walk stays in step.
        static std::shared_ptr<FDeferredByteData> ReadDeferredByteData(
            Readers::FArchive& Ar, const WwiseDataSource& source, int64_t offset, int32_t size);

        /// Reads only the SoundBankId from a .bnk file
        /// In order to quickly find the SoundBank without parsing the entire file
        static std::optional<uint32_t> TryReadSoundBankId(Readers::FArchive& Ar);

    private:
        const WwiseDataSource* _source = nullptr;
    };
}
