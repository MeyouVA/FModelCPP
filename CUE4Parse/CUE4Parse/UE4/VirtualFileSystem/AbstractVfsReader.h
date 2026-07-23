// Ported from CUE4Parse/UE4/VirtualFileSystem/AbstractVfsReader.cs
//
// Deliberate differences from C#:
//   * MountPoint is a protected field here rather than an abstract property with a protected setter — every
//     C# implementation backs it with a plain field anyway, and an abstract property buys nothing in C++.
//   * The Serilog warnings are dropped (the port has no logging layer), as is Globals.LogVfsMounts which
//     only ever gated them.
//   * The static Write(char[], ...) span helpers are IoStore-only and arrive with that reader. TODO.
//   * C#'s EventHandler<int> callbacks become a single std::function (no multicast).
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "IVfsReader.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::FileProvider::Vfs { class FileProviderDictionary; }

namespace CUE4Parse::UE4::VirtualFileSystem
{
    // C#'s EventHandler<int> as used by the provider events: (sender reader, count).
    using VfsEventHandler = std::function<void(IVfsReader&, int)>;

    class AbstractVfsReader : public virtual IVfsReader
    {
    public:
        const std::string& Path() const override { return _path; }
        const std::string& Name() const override { return _name; }
        int64_t ReadOrder() const override { return _readOrder; }

        const GameFileMap& Files() const override { return _files; }

        const std::string& MountPoint() const override { return _mountPoint; }

        bool IsConcurrent() const override { return _isConcurrent; }
        void SetConcurrent(bool value) override { _isConcurrent = value; }

        Versions::VersionContainer& GetVersions() override { return _versions; }
        Versions::EGame Game() const override { return _versions.Game(); }
        Versions::FPackageFileVersion Ver() const override { return _versions.Ver(); }

        std::string ToString() const { return _path; }

        static constexpr int MAX_MOUNTPOINT_TEST_LENGTH = 128;

        // Cheap "does this decrypt to something that starts with a plausible mount point?" probe. Used to
        // decide whether an AES key is the right one before committing to it.
        static bool IsValidIndex(const std::vector<uint8_t>& testBytes);
        static bool IsValidIndex(Readers::FArchive& reader);

        // From AesVfsReaderForProvider.cs: mounts and registers this reader's files with the provider's
        // dictionary. Defined in AesVfsReaderForProvider.cpp.
        void MountTo(FileProvider::Vfs::FileProviderDictionary& files, const Utils::StringComparer& pathComparer,
                     const VfsEventHandler& vfsMounted = nullptr);

    protected:
        AbstractVfsReader(std::string path, Versions::VersionContainer versions);

        // Normalises a raw mount point in place, falling back to root when it looks wrong, then recomputes
        // ReadOrder. C# takes `ref string`; a reference does the same here.
        void ValidateMountPoint(std::string& mountPoint);

        // The C# GameFileMap needs its comparator at construction, so subclasses build the map with the
        // comparer Mount was handed and hand it over here.
        GameFileMap _files{Utils::StringComparer::Ordinal()};
        std::string _mountPoint;
        Versions::VersionContainer _versions;

    private:
        void VerifyReadOrder();
        static int GetPakOrderFromPakFilePath();

        std::string _path;
        std::string _name;
        int64_t _readOrder = 0;
        bool _isConcurrent = false;
    };
}
