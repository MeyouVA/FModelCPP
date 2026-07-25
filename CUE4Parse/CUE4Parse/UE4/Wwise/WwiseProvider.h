// Ported from CUE4Parse/UE4/Wwise/WwiseProvider.cs
// The layer above WwiseReader: it mounts every soundbank a game ships, indexes their HIRC hierarchies into
// one flat id -> node table, and then answers "which .wem files does this event/bank play?" by walking that
// table. WwiseReader parses one container; this parses all of them and joins them up.
//
// Two independent routes reach the same answer, and which one runs depends on how the game was cooked:
//   * the *cooked-data* route -- a modern UAkAudioEvent carries an FWwiseLocalizedEventCookedData naming its
//     media and banks outright, so the media list is read straight off the export;
//   * the *hierarchy* route -- an older event carries only a ShortID (or nothing, in which case the event's
//     own name is FNV-hashed into one), and the answer has to be found by traversing the bank hierarchy
//     from that event id down through actions, containers and music segments to the leaf sources.
//
// Deliberate differences from C#:
//   * Audio is never decoded. C# hands back FDeferredByteData and so does this -- a WwiseExtractedSound is a
//     name plus a promise of bytes, and turning a .wem into a .wav is somebody else's job.
//   * OWNERSHIP: C# keeps GC references to the WwiseReaders it parses and to the Hierarchy objects inside
//     them. Here the provider owns every reader it creates (_ownedReaders) and the hierarchy tables hold
//     *non-owning* pointers into them; the readers that come from an asset library instead belong to the
//     package the provider cached, which also outlives it. Media are shared_ptr<FDeferredByteData>, so those
//     have their own lifetime either way.
//   * No logging. C# warns on a soundbank that cannot be found or read, on a .wem whose bytes are missing,
//     and on an unhandled hierarchy type; the control flow those warnings accompany is kept exactly.
//   * C#'s local functions (TraverseAndSave / SaveWemSound / TraverseSwitchContainer / TraverseDecisionTreeNode)
//     become private members plus an EventWalk struct holding what those closures captured, since a C++
//     lambda cannot recurse without that ceremony.
//   * `Path.Combine` is spelled out as a join with '/', following the convention already set in AkEntry.h.
//     C#'s subsequent Replace('\\','/') calls are kept: cooked path names can still contain backslashes.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "FDeferredByteData.h"
#include "WwiseReader.h"
#include "Objects/AkDecisionTree.h"
#include "Objects/Actions/CAkActionSetSwitch.h"
#include "Objects/HIRC/Containers/HierarchySwitchContainer.h"
#include "Objects/HIRC/Hierarchy.h"
#include "../Assets/Exports/UObject.h"
#include "../Assets/Exports/Wwise/FWwiseEventCookedData.h"
#include "../Assets/Exports/Wwise/FWwiseLanguageCookedData.h"
#include "../Assets/Exports/Wwise/FWwiseMediaCookedData.h"
#include "../Assets/Exports/Wwise/FWwiseSoundBankCookedData.h"
#include "../Assets/Exports/Wwise/UAkAudioBank.h"
#include "../Assets/Exports/Wwise/UAkAudioEvent.h"
#include "../../FileProvider/Objects/GameFile.h"
#include "../../FileProvider/Vfs/AbstractVfsFileProvider.h"

namespace CUE4Parse::UE4::Wwise
{
    using CUE4Parse::FileProvider::Objects::GameFile;
    using CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider;
    using CUE4Parse::UE4::Assets::Exports::Wwise::FWwiseEventCookedData;
    using CUE4Parse::UE4::Assets::Exports::Wwise::FWwiseLanguageCookedData;
    using CUE4Parse::UE4::Assets::Exports::Wwise::FWwiseMediaCookedData;
    using CUE4Parse::UE4::Assets::Exports::Wwise::FWwiseSoundBankCookedData;
    using CUE4Parse::UE4::Assets::Exports::Wwise::UAkAudioBank;
    using CUE4Parse::UE4::Assets::Exports::Wwise::UAkAudioEvent;
    using CUE4Parse::UE4::Wwise::Objects::AkDecisionTreeNode;
    using CUE4Parse::UE4::Wwise::Objects::Actions::CAkActionSetSwitch;
    using CUE4Parse::UE4::Wwise::Objects::HIRC::Hierarchy;
    using CUE4Parse::UE4::Wwise::Objects::HIRC::Containers::HierarchySwitchContainer;

    // One extractable sound: where it should be written and how to get its bytes.
    struct WwiseExtractedSound
    {
        std::string OutputPath;
        std::string Extension;
        std::shared_ptr<FDeferredByteData> Data;

        std::vector<uint8_t> GetData() const { return Data != nullptr ? Data->GetData() : std::vector<uint8_t>(); }
        std::string ToString() const;
    };

    class WwiseProvider
    {
    public:
        // Mirrors C#'s constructor exactly, side effects and all: it loads the multi-reference asset
        // libraries, bulk-parses every bank the provider can see, and throws when that found nothing.
        WwiseProvider(AbstractVfsFileProvider& provider, std::string gameDirectory);

        // Please don't change this, when extracting directly from .bnk we shouldn't loop through wwise hierarchy
        // because that doesn't guarantee us to extract the audio from this given soundbank
        std::vector<WwiseExtractedSound> ExtractBankSounds(WwiseReader& wwiseReader);
        std::vector<WwiseExtractedSound> ExtractBankSounds(UAkAudioBank& audioBank);
        std::vector<WwiseExtractedSound> ExtractAudioEventSounds(UAkAudioEvent& audioEvent);

        std::string GetOwnerDirectory(const Assets::Exports::UObject& obj) const;

    private:
        // What C#'s LoopThroughEvent local functions captured, carried explicitly so the traversal members
        // can recurse.
        struct EventWalk
        {
            std::vector<WwiseExtractedSound>* Results = nullptr;
            std::string OwnerDirectory;
            std::string DebugName;
            uint32_t EventId = 0;
            std::vector<CAkActionSetSwitch> SwitchStates;
        };

        void ProcessMediaCookedData(const std::string& ownerDirectory, const FWwiseMediaCookedData& media,
                                    const FWwiseLanguageCookedData& languageData,
                                    std::vector<WwiseExtractedSound>& results);
        void CacheSoundBankCookedData(const FWwiseSoundBankCookedData& soundBank);
        void CacheMediaCookedData(const FWwiseMediaCookedData& media);
        void ProcessSoundBankCookedData(const std::string& ownerDirectory, const FWwiseEventCookedData& eventData,
                                        std::vector<WwiseExtractedSound>& results,
                                        const std::unordered_set<uint32_t>& visitedMedia);

        WwiseReader* LoadSoundBankById(uint32_t soundBankId, bool returnBank = false);

        void LoopThroughEvent(uint32_t eventId, std::vector<WwiseExtractedSound>& results,
                              const std::string& ownerDirectory, const std::string& debugName = std::string());
        void LoopThroughEvent(uint32_t eventId, std::vector<WwiseExtractedSound>& results,
                              const std::string& ownerDirectory, const std::unordered_set<uint32_t>& visitedMedia,
                              const std::string& debugName = std::string());
        void TraverseAndSave(EventWalk& walk, uint32_t id);
        void TraverseDecisionTreeNode(EventWalk& walk, const AkDecisionTreeNode& node);
        void TraverseSwitchContainer(EventWalk& walk, const HierarchySwitchContainer& switchContainer,
                                     uint32_t stateId);
        void SaveWemSound(EventWalk& walk, uint32_t wemId);

        int LoadExternalWwiseFiles();
        void BulkInitializeWwise();
        bool TryLoadAndCacheWwiseFile(const std::shared_ptr<GameFile>& gameFile);
        void CacheWwiseFile(WwiseReader& wwiseReader);
        std::vector<std::string> LoadWwisePackagingSettings();
        void LoadMultiReferenceLibrary();
        void TryLoadMultiReferenceAsset(const std::shared_ptr<GameFile>& assetFile);
        std::vector<const Hierarchy*> GetHierarchiesById(uint32_t id) const;

        AbstractVfsFileProvider& _provider;
        std::string _gameDirectory;
        std::string _baseWwiseAudioPath;

        // C#'s static HashSet<string>(OrdinalIgnoreCase). Extensions off a GameFile are already lower-case in
        // practice; the comparisons below fold case explicitly where C# relied on the comparer.
        static const std::unordered_set<std::string>& ValidWwiseExtensions();

        std::unordered_map<uint32_t, const Hierarchy*> _wwiseHierarchyTables;
        std::unordered_map<uint32_t, std::vector<const Hierarchy*>> _wwiseHierarchyDuplicates;
        std::map<std::string, std::shared_ptr<FDeferredByteData>> _wwiseEncodedMedia;
        std::unordered_set<uint32_t> _wwiseLoadedSoundBanks;
        std::unordered_map<uint32_t, WwiseReader*> _multiReferenceLibraryCache;
        bool _completedWwiseFullBnkInit = false;
        bool _loadedMultiRefLibrary = false;
        int64_t _totalLoadedWwiseSize = 0;
        int64_t _totalWwiseBanksSize = 0;

        std::unordered_map<uint32_t, std::shared_ptr<FGameFileDeferredByteData>> _looseWemFilesLookup;

        std::set<std::pair<uint32_t, const Hierarchy*>> _visitedHierarchies; // To speed things up
        std::unordered_set<uint32_t> _visitedWemIds;                        // To prevent duplicates

        // Not in C#, where the GC keeps these alive: every WwiseReader this provider parses itself. The
        // hierarchy tables point into them (see the OWNERSHIP note above), so they must outlive the tables.
        std::vector<std::unique_ptr<WwiseReader>> _ownedReaders;
    };
}
