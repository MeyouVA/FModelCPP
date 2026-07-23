// Ported from CUE4Parse/FileProvider/Objects/GameFile.cs
// The abstract "a file inside a mounted container" base. Path derivatives (directory, name, extension) are
// computed lazily and invalidated when Path is reassigned, exactly as in C#.
//
// Deliberate differences from C#:
//   * The FByteBulkDataHeader parameter on Read/CreateReader is dropped: that type belongs to the asset
//     bulk-data layer, which is not ported. TODO: restore the parameter with that layer, at which point
//     PakFileReader::Extract's partial-read branch becomes reachable.
//   * The async wrappers (ReadAsync/SafeReadAsync/...) are omitted — Task.Run over a synchronous body is a
//     C#-ism, and the port has no threading layer yet.
//   * Extension interning (the ConcurrentDictionary) is dropped: std::string already owns its bytes and the
//     dedup only ever bought C# heap savings.
//   * TryRead/TryCreateReader return the value or nullopt/null instead of a bool + out param, and log
//     nothing (the port has no Serilog equivalent).
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "../../Compression/CompressionMethod.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::FileProvider::Objects
{
    class GameFile
    {
    public:
        static const std::vector<std::string> UePackageExtensions;
        static const std::vector<std::string> UePackagePayloadExtensions;
        static const std::vector<std::string> UeKnownExtensions;

        virtual ~GameFile() = default;

        virtual bool IsEncrypted() const = 0;
        virtual Compression::CompressionMethod GetCompressionMethod() const = 0;

        const std::string& Path() const { return _path; }
        void SetPath(std::string value)
        {
            _path = std::move(value);
            _directory.reset();
            _pathWithoutExtension.reset();
            _name.reset();
            _nameWithoutExtension.reset();
            _extension.reset();
        }

        int64_t Size = 0;

        const std::string& Directory() const { return Cache(_directory, [&] { return Utils::SubstringBeforeLast(_path, '/'); }); }
        const std::string& PathWithoutExtension() const { return Cache(_pathWithoutExtension, [&] { return Utils::SubstringBeforeLast(_path, '.'); }); }
        const std::string& Name() const { return Cache(_name, [&] { return Utils::SubstringAfterLast(_path, '/'); }); }
        const std::string& NameWithoutExtension() const { return Cache(_nameWithoutExtension, [&] { return Utils::SubstringBeforeLast(Name(), '.'); }); }
        const std::string& Extension() const { return Cache(_extension, [&] { return Utils::SubstringAfterLast(Name(), '.'); }); }

        bool IsUePackage() const { return IsKnown(UePackageExtensionsSet(), Extension()); }
        bool IsUePackagePayload() const { return IsKnown(UePackagePayloadExtensionsSet(), Extension()); }

        virtual std::vector<uint8_t> Read() = 0;
        virtual std::unique_ptr<UE4::Readers::FArchive> CreateReader() = 0;

        // C#'s TryRead/SafeRead collapse into one: nullopt means the read threw.
        std::optional<std::vector<uint8_t>> SafeRead();
        std::unique_ptr<UE4::Readers::FArchive> SafeCreateReader();

        std::string ToString() const { return _path; }

        static const std::unordered_set<std::string>& UePackageExtensionsSet();
        static const std::unordered_set<std::string>& UePackagePayloadExtensionsSet();
        static const std::unordered_set<std::string>& UeKnownExtensionsSet();

    protected:
        GameFile() = default;
        GameFile(std::string path, int64_t size) : Size(size), _path(std::move(path)) {}

    private:
        template <typename Fn>
        static const std::string& Cache(std::optional<std::string>& slot, Fn compute)
        {
            if (!slot.has_value()) slot = compute();
            return *slot;
        }

        // The C# sets are built with StringComparer.OrdinalIgnoreCase; here the lookup lowercases instead.
        static bool IsKnown(const std::unordered_set<std::string>& set, const std::string& extension);

        std::string _path;
        mutable std::optional<std::string> _directory;
        mutable std::optional<std::string> _pathWithoutExtension;
        mutable std::optional<std::string> _name;
        mutable std::optional<std::string> _nameWithoutExtension;
        mutable std::optional<std::string> _extension;
    };
}
