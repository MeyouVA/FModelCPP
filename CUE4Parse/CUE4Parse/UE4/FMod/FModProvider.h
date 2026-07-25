// Ported from CUE4Parse/UE4/FMod/FModProvider.cs
// The layer above FModReader, and the FMOD counterpart of WwiseProvider: it finds every .bank a game ships
// (inside the paks and loose on disk beside them), merges the ones that belong together, resolves each
// event's node graph down to the waveforms it plays, and answers "which sounds does this UFMODEvent /
// UFMODBank use?".
//
// The merging matters and is not incidental: FMOD splits one logical bank across several files -- the
// metadata in `Foo.bank`, the audio in `Foo.assets.bank`, localised audio in `Foo.<lang>.bank`. C# groups
// them by the file name up to the first '.', merges each group into one reader, and then merges again by
// bank GUID so pak and on-disk copies of the same bank land together. All of that is kept.
//
// Deliberate differences from C#:
//   * Audio is never decoded. C#'s FModExtractedSound carries the bytes Fmod5Sharp's
//     RebuildAsStandardFileFormat produced; that decoder is out of scope (see FModSoundBank.h), so the port
//     hands back the sample's identity instead -- see Utils/FWaveformRef.h. Consequently:
//       - the C# `Extension` field has no counterpart (it is whatever the decoder decided to emit), and
//       - `Name` is always the `{fallback}_{i}` form, since the real sample names live in the FSB5 name
//         table that only the decoder reads. C#'s `sample.Name ?? $"{fallback}_{i}"` fallback is what
//         survives.
//     The FSB5 container bytes are reachable through SoundBanks(), so a real decoder is a local change.
//   * The encryption key stays a *static* field, as in C#. It is genuinely global there (a bank being read
//     anywhere picks it up), and FModReader's own key parameter defaults from the same place.
//   * No logging; the failure paths C# logs are kept, just silent.
//   * `#if DEBUG` blocks (LogMissingSamples, the "encryption key not found" note) are omitted with the
//     logging layer.
//   * C# takes an `IFileProvider`, whose interface declares the Files dictionary, the config inis and
//     TrySaveAsset. This port's IFileProvider is a much slimmer interface (see IFileProvider.h) and those
//     three live on AbstractFileProvider, so that is what is taken here -- which is what every real caller
//     passes anyway.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "FModReader.h"
#include "Objects/FModGuid.h"
#include "Utils/FWaveformRef.h"
#include "../Assets/Exports/FMod/UFMODBank.h"
#include "../Assets/Exports/FMod/UFMODEvent.h"
#include "../../FileProvider/AbstractFileProvider.h"

namespace CUE4Parse::UE4::FMod
{
    using CUE4Parse::FileProvider::AbstractFileProvider;
    using CUE4Parse::UE4::Assets::Exports::FMod::UFMODBank;
    using CUE4Parse::UE4::Assets::Exports::FMod::UFMODEvent;
    using CUE4Parse::UE4::FMod::Objects::FModGuid;
    using CUE4Parse::UE4::FMod::Utils::FWaveformRef;

    // One extractable sound. See the header note: C#'s Extension/Data are the decoder's output, so what is
    // here instead is which subsound of which FSB5 container the sample is.
    struct FModExtractedSound
    {
        std::string Name;
        FWaveformRef Waveform;
        // The container the waveform lives in; borrowed from the FModReader that owns it (which the
        // provider owns), or null when the index did not resolve.
        const FModSoundBank* SoundBank = nullptr;

        std::string ToString() const { return Name; }
    };

    class FModProvider
    {
    public:
        // Mirrors C#'s constructor: settings, then pak banks, then on-disk banks, then the event cache.
        FModProvider(AbstractFileProvider& provider, const std::string& gameDirectory);

        // C#'s TryLoadBank(Stream, string, out FModReader?): null on failure instead of a bool + out param.
        std::unique_ptr<FModReader> TryLoadBank(Readers::FArchive& Ar, const std::string& bankName) const;

        void UpdateEventCache();

        std::vector<FModExtractedSound> ExtractEventSounds(UFMODEvent& audioEvent);
        std::vector<FModExtractedSound> ExtractBankSounds(UFMODBank& audioBank);
        std::vector<FModExtractedSound> ExtractBankSoundTable(const FModReader& fmodReader);
        std::vector<FModExtractedSound> ExtractBankSounds(const FModReader& fmodReader);

        // Not in C#: the merged readers, so a caller can reach the raw FSB5 bytes a FWaveformRef names.
        const std::unordered_map<FModGuid, std::unique_ptr<FModReader>>& MergedReaders() const { return _mergedReaders; }

    private:
        void LoadPakBanks(AbstractFileProvider& provider);
        void LoadFileBanks(std::string gameDirectory);
        void LoadFModSettings(AbstractFileProvider& provider);

        // C#'s `_mergedReaders[guid] = mergedBank` / `existing.Merge(mergedBank)` tail, shared by both loaders.
        void MergeIntoCache(std::unique_ptr<FModReader> mergedBank);

        std::vector<FModExtractedSound> ExtractAudioSamples(const std::vector<FWaveformRef>& samples,
                                                            const std::string& fallbackSampleName,
                                                            const FModReader& owner) const;

        std::unordered_map<FModGuid, std::vector<FWaveformRef>> _resolvedEventsCache;
        std::unordered_map<FModGuid, bool> _eventResolutionStatus;
        std::unordered_map<FModGuid, FModGuid> _eventToReaderMap;
        // Owns its readers, which C# leaves to the GC; FModExtractedSound::SoundBank borrows into them.
        std::unordered_map<FModGuid, std::unique_ptr<FModReader>> _mergedReaders;
        static std::optional<std::vector<uint8_t>> _encryptionKey;
        std::string _bankOutputDirectory;
    };
}
