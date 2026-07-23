// Ported from CUE4Parse/FileProvider/Vfs/IVfsFileProvider.cs (portable subset).
// The provider surface for container-backed (pak/utoc) providers.
//
// Deliberate differences from C#:
//   * Only the members whose layers exist are declared: the register/mount/key lifecycle and the archive
//     lookups. GlobalData/FilesById/LoadPackage(FPackageId) arrive with the IO Store layer, PostMount with
//     the config-ini layer. TODO.
//   * The C# events are plain std::function members on the implementation (no multicast), so they are not
//     part of the interface.
//   * StringComparison parameters become Utils::StringComparer.
#pragma once

#include <memory>
#include <vector>

#include "../IFileProvider.h"
#include "../../Encryption/Aes/FAesKey.h"
#include "../../UE4/Objects/Core/Misc/FGuid.h"
#include "../../UE4/VirtualFileSystem/IAesVfsReader.h"
#include "../../Utils/StringComparer.h"

namespace CUE4Parse::FileProvider::Vfs
{
    using UE4::Objects::Core::Misc::FGuid;
    using UE4::VirtualFileSystem::IAesVfsReader;

    // FGuid as an ordered-map key (C# uses it as a Dictionary key via GetHashCode; std::map needs an order).
    struct FGuidLess
    {
        bool operator()(const FGuid& a, const FGuid& b) const
        {
            if (a.A != b.A) return a.A < b.A;
            if (a.B != b.B) return a.B < b.B;
            if (a.C != b.C) return a.C < b.C;
            return a.D < b.D;
        }
    };

    class IVfsFileProvider : public virtual IFileProvider
    {
    public:
        // Scan the given source (directory, streams, ...) for archives to register.
        virtual void Initialize() = 0;

        virtual void RegisterVfs(const std::string& file) = 0;

        // Mounts every registered archive that needs no AES key; returns the number of new mounts.
        virtual int Mount() = 0;
        // Mounts every registered archive whose EncryptionKeyGuid matches; returns the number of new mounts.
        virtual int SubmitKey(const FGuid& guid, const std::shared_ptr<Encryption::Aes::FAesKey>& key) = 0;
        virtual int SubmitKeys(const std::vector<std::pair<FGuid, std::shared_ptr<Encryption::Aes::FAesKey>>>& keys) = 0;

        virtual IAesVfsReader* TryGetArchive(const std::string& archiveName,
                                             const Utils::StringComparer& comparison = Utils::StringComparer::Ordinal()) = 0;
    };
}
