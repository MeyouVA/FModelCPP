// Ported from CUE4Parse/UE4/Pak/PakFileReader.cs
// Mounts a .pak: reads the trailer, walks whichever index format it declares, and extracts (decrypting and
// decompressing) file data on demand.
//
// Deliberate differences from C#:
//   * Every per-game hook that lives in CUE4Parse.GameTypes is omitted, because none of GameTypes is ported:
//     the specialised Extract paths (PartialEncrypt/GameForPeace/Rennsport/DQXI/Century/ABI/ProSpi), the
//     per-game Lua/ini/csv post-decryptors, and the CoA/DragonSword/ValorantSource/GameForPeace/DQXI index
//     readers. Where C# would dispatch to one of those, this throws with a message naming the game rather
//     than silently producing wrong bytes. The mainline UE paths — legacy, updated, flat and frozen indices,
//     compressed and uncompressed extraction — are ported in full. TODO with the GameTypes layer.
//   * The FByteBulkDataHeader parameter is dropped along with the type (see GameFile.h), so Extract always
//     returns the whole entry; the partial-read arithmetic is kept intact around a zero offset so restoring
//     the parameter is a local change.
//   * C# reads the updated index's directory names through GenericReader's ReadFStringMemory; FArchive's
//     ReadFString reads the same length-prefixed encoding (negative length = UTF-16) and is used instead.
//   * Stopwatch/Serilog reporting in Mount is dropped (no logging layer).
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Objects/FPakEntry.h"
#include "Objects/FPakInfo.h"
#include "../Readers/FArchive.h"
#include "../VirtualFileSystem/AbstractAesVfsReader.h"

namespace CUE4Parse::UE4::Pak
{
    using VirtualFileSystem::GameFileMap;

    class PakFileReader : public VirtualFileSystem::AbstractAesVfsReader
    {
    public:
        // C# holds `readonly FArchive Ar`; shared_ptr because the reader owns the archive it was handed.
        std::shared_ptr<Readers::FArchive> Ar;
        Objects::FPakInfo Info;

        explicit PakFileReader(std::shared_ptr<Readers::FArchive> ar);
        // Opens `filePath` as a file archive, like C#'s string/FileInfo constructors.
        explicit PakFileReader(const std::string& filePath, Versions::VersionContainer versions = Versions::VersionContainer());

        int64_t Length() const override { return _length; }
        bool HasDirectoryIndex() const override { return true; }
        UE4::Objects::Core::Misc::FGuid EncryptionKeyGuid() const override { return Info.EncryptionKeyGuid; }
        bool IsEncrypted() const override { return Info.EncryptedIndex; }

        std::vector<uint8_t> Extract(VirtualFileSystem::VfsEntry& entry) override;
        void Mount(const Utils::StringComparer& pathComparer) override;
        std::vector<uint8_t> MountPointCheckBytes() override;

    protected:
        std::vector<uint8_t> ReadAndDecrypt(int length) override;
        std::vector<uint8_t> ReadAndDecryptIndex(int length) override;

    private:
        // These games use version >= 12 to indicate their custom formats
        bool UsingCustomPakVersion() const;

        void ReadIndexLegacy(const Utils::StringComparer& pathComparer);
        void ReadIndexUpdated(const Utils::StringComparer& pathComparer);
        void ReadFlatDirectoryIndex(Readers::FArchive& directoryIndex, GameFileMap& files,
                                    Readers::FArchive& encodedPakEntries,
                                    std::vector<std::shared_ptr<Objects::FPakEntry>>& nonEncodedEntries);
        void ReadFrozenIndex(const Utils::StringComparer& pathComparer);

        int64_t _length = 0;
    };
}
