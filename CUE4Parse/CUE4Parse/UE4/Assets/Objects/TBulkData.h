// Ported from CUE4Parse/UE4/Assets/Objects/TBulkData.cs
// A lazily-read array of T stored out of line from the export that owns it. The header says where the
// payload lives -- inline right here, at the end of this file, or in a .ubulk/.uptnl/.m.ubulk sidecar --
// and GetBulkArchive resolves that into "an archive plus a position" the read then works from.
//
// Deliberate differences from C#:
//   * C#'s Lazy<T[]?> becomes an explicit std::optional cache filled on first Data() call. Same laziness,
//     no allocation for the thunk.
//   * GetBulkArchive returns a small BulkArchiveRef rather than an out-param, because two of its arms hand
//     back a *borrowed* archive (the saved one) and three hand back one this port has to own (a payload
//     archive created on the spot). C# lets the GC paper over that difference.
//   * ArrayPool is dropped -- a plain vector; the pool was a C# allocation optimisation with no semantics.
//   * No logging: C# warns on a short read and Log.Debug's each failed payload lookup. The behaviour those
//     accompany (carry on with whatever was read / return false) is kept exactly.
#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "EBulkDataFlags.h"
#include "FByteBulkDataHeader.h"
#include "../Readers/FAssetArchive.h"
#include "../Utils/PayloadType.h"
#include "../../Exceptions/ParserException.h"
#include "../../Readers/FByteArchive.h"
#include "../../Objects/Core/Misc/ECompressionFlags.h"
#include "../IPackage.h"
#include "../../../FileProvider/IFileProvider.h"
#include "../../../FileProvider/Objects/GameFile.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Assets::Utils::PayloadType;
    using CUE4Parse::UE4::Readers::FByteArchive;
    using CUE4Parse::UE4::Objects::Core::Misc::ECompressionFlags;
    using CUE4Parse::UE4::Versions::VersionContainer;

    // The resolved "where do I read the payload from" answer. Archive is always the one to read through;
    // Owned is non-null only when this reference created it (a sidecar payload archive), in which case it
    // keeps it alive for as long as the reference is in scope.
    struct BulkArchiveRef
    {
        FAssetArchive* Archive = nullptr;
        int64_t Position = 0;
        std::unique_ptr<FAssetArchive> Owned;

        explicit operator bool() const { return Archive != nullptr; }
    };

    template <typename T>
    class TBulkData
    {
        static_assert(std::is_trivially_copyable_v<T>, "TBulkData<T> requires a trivially copyable T");

    public:
        FByteBulkDataHeader Header;

        EBulkDataFlags BulkDataFlags() const { return Header.BulkDataFlags; }

        virtual ~TBulkData() = default;

        // C#'s Data property: reads on first touch, then hands back the cache. Null means the read failed.
        const std::vector<T>* Data() const
        {
            if (!_dataEvaluated)
            {
                _dataEvaluated = true;
                std::vector<T> data;
                if (_readsEmpty) _data = std::vector<T>();
                else if (ReadBulkDataInto(data)) _data = std::move(data);
            }
            return _data.has_value() ? &*_data : nullptr;
        }

        /// Returns the size of the bulk data in bytes.
        virtual int GetDataSize() const { return Header.ElementCount * static_cast<int>(sizeof(T)); }

    protected:
        TBulkData() = default;

        explicit TBulkData(std::vector<T> data)
        {
            _data = std::move(data);
            _dataEvaluated = true;
        }

        explicit TBulkData(FAssetArchive& Ar)
        {
            Header = FByteBulkDataHeader(Ar);
            if (Header.SizeOnDisk == 0 || HasFlag(BulkDataFlags(), EBulkDataFlags::BULKDATA_Unused))
            {
                _readsEmpty = true;
                return;
            }

            _dataPosition = Ar.Position;
            _savedAr = &Ar;

            // Inline (and the two "no separate file" flag shapes) means the payload sits right here, so the
            // archive has to be stepped past it; every other shape leaves the cursor where it is.
            if (HasFlag(BulkDataFlags(), EBulkDataFlags::BULKDATA_ForceInlinePayload) ||
                BulkDataFlags() == EBulkDataFlags::BULKDATA_LazyLoadable ||
                BulkDataFlags() == EBulkDataFlags::BULKDATA_None)
            {
                Ar.Position += Header.SizeOnDisk;
            }
        }

        virtual bool ReadBulkDataInto(std::vector<T>& data) const
        {
            data.clear();
            if (_savedAr == nullptr) return false;

            BulkArchiveRef ref = GetBulkArchive();
            if (!ref) return false;

            std::vector<uint8_t> bulkData(static_cast<size_t>(Header.SizeOnDisk), 0);
            // C# warns when this comes up short; the port carries on with what it got, as C# does.
            ref.Archive->ReadAt(ref.Position, bulkData.data(), 0, static_cast<int>(Header.SizeOnDisk));

            FByteArchive dataAr("", std::move(bulkData), _savedAr->Versions);
            if (HasFlag(BulkDataFlags(), EBulkDataFlags::BULKDATA_SerializeCompressedZLIB))
            {
                const int size = GetDataSize();
                data.assign(static_cast<size_t>(Header.ElementCount), T{});
                std::vector<uint8_t> uncompressedData(static_cast<size_t>(size));
                int64_t partialReadLength = 0; // C#'s `out _`
                dataAr.SerializeCompressedNew(uncompressedData.data(), size, "Zlib",
                                              ECompressionFlags::COMPRESS_NoFlags, false, partialReadLength);
                if (size > 0) std::memcpy(data.data(), uncompressedData.data(), static_cast<size_t>(size));
            }
            else
            {
                data = dataAr.ReadArray<T>(Header.ElementCount);
            }

            return true;
        }

        BulkArchiveRef GetBulkArchive() const
        {
            BulkArchiveRef ref;
            ref.Archive = _savedAr;
            ref.Position = _dataPosition;
            if (ref.Archive == nullptr) return ref;

            const EBulkDataFlags flags = BulkDataFlags();
            if (HasFlag(flags, EBulkDataFlags::BULKDATA_ForceInlinePayload))
            {
                // Payload is inline at _dataPosition; nothing to redirect.
            }
            else if (HasFlag(flags, EBulkDataFlags::BULKDATA_OptionalPayload))
            {
                ref.Owned = TryGetBulkPayload(*_savedAr, PayloadType::UPTNL);
                if (ref.Owned == nullptr) { ref.Archive = nullptr; return ref; }
                ref.Archive = ref.Owned.get();
                ref.Position = ref.Archive->Length == Header.SizeOnDisk ? 0 : Header.OffsetInFile;
            }
            else if (HasFlag(flags, EBulkDataFlags::BULKDATA_PayloadInSeperateFile | EBulkDataFlags::BULKDATA_MemoryMappedPayload))
            {
                ref.Owned = TryGetBulkPayload(*_savedAr, PayloadType::MUBULK);
                if (ref.Owned == nullptr) { ref.Archive = nullptr; return ref; }
                ref.Archive = ref.Owned.get();
                ref.Position = ref.Archive->Length == Header.SizeOnDisk ? 0 : Header.OffsetInFile;
            }
            else if (HasFlag(flags, EBulkDataFlags::BULKDATA_PayloadInSeperateFile))
            {
                ref.Owned = TryGetBulkPayload(*_savedAr, PayloadType::UBULK);
                if (ref.Owned == nullptr) { ref.Archive = nullptr; return ref; }
                ref.Archive = ref.Owned.get();
                ref.Position = ref.Archive->Length == Header.SizeOnDisk ? 0 : Header.OffsetInFile;
            }
            else if (HasFlag(flags, EBulkDataFlags::BULKDATA_PayloadAtEndOfFile))
            {
                if (Header.OffsetInFile + Header.SizeOnDisk > ref.Archive->Length)
                    throw Exceptions::ParserException(*ref.Archive,
                        "Failed to read PayloadAtEndOfFile, " + std::to_string(Header.OffsetInFile) + " is out of range");

                // stored in same file, but at different position
                ref.Position = Header.OffsetInFile;
            }
            else if (HasFlag(flags, EBulkDataFlags::BULKDATA_LazyLoadable) || HasFlag(flags, EBulkDataFlags::BULKDATA_None))
            {
                //
            }

            return ref;
        }

        FAssetArchive* _savedAr = nullptr;
        int64_t _dataPosition = 0;

        // Mutable so the C# `Data` property stays a const read that fills its cache on first touch.
        mutable std::optional<std::vector<T>> _data;
        mutable bool _dataEvaluated = false;
        bool _readsEmpty = false;

    private:
        // C#'s TryGetBulkPayload. With the default cooked index the payload is whatever the package
        // registered under that PayloadType (except .m.ubulk, which is always looked up by path); with a
        // non-default one the sidecar is named "<asset>.NNN.<ext>" and has to be found through the provider.
        std::unique_ptr<FAssetArchive> TryGetBulkPayload(FAssetArchive& Ar, PayloadType type) const
        {
            const auto openByPath = [&](const std::string& path) -> std::unique_ptr<FAssetArchive>
            {
                auto* provider = Ar.Owner != nullptr ? Ar.Owner->GetProvider() : nullptr;
                if (provider == nullptr) return nullptr;
                auto file = provider->TryGetGameFile(path);
                if (file == nullptr) return nullptr;
                auto reader = file->SafeCreateReader(&Header);
                if (reader == nullptr) return nullptr;
                // The FAssetArchive owns the reader it was handed, so the payload outlives this call.
                return FAssetArchive::CreateOwning(std::move(reader), Ar.Owner);
            };

            if (Header.CookedIndex.IsDefault())
            {
                if (type == PayloadType::MUBULK)
                    return openByPath(ChangeExtension(Ar.Name(), ".m.ubulk"));
                return Ar.TryGetPayload(type, &Header);
            }

            return openByPath(ChangeExtension(
                Ar.Name(), "." + Header.CookedIndex.GetAsExtension() + "." + Utils::ToExtension(type)));
        }

        // Path.ChangeExtension: replace everything from the last '.' of the file name.
        static std::string ChangeExtension(const std::string& path, const std::string& newExtension)
        {
            const size_t slash = path.find_last_of("/\\");
            const size_t dot = path.find_last_of('.');
            if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
                return path + newExtension;
            return path.substr(0, dot) + newExtension;
        }

    };
}
