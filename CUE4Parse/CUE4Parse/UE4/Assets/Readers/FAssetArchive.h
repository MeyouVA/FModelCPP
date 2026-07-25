// Ported from CUE4Parse/UE4/Assets/Readers/FAssetArchive.cs
// An archive layered over a base reader that knows its owning package: its ReadFName resolves indices
// against the package name pool, and it tracks an AbsoluteOffset used to translate package-relative
// offsets to base-archive offsets.
//
// Deliberate differences from C#:
//   * C# delegates the Position/Length *properties* to the base archive. This port keeps Position/Length
//     as plain archive state (the project-wide design) and treats the base as a random-access byte store
//     addressed by this archive's Position — so direct `Ar.Position += n` skips work without desyncing.
//     A consequence: typed reads go through this archive (little-endian), not the base's Read<T>, so a
//     big-endian base would not swap here. Asset archives are little-endian, matching the rest of the
//     reader design (composites are read field-by-field).
//   * Object resolution/loading (ReadObject<T>/ReadUObject) is deferred with the asset-export layer.
//     TODO: bring it over then.
//   * The bulk-data payload subsystem IS ported, with one ownership change: C#'s dictionary maps a
//     PayloadType to a Func returning an FAssetArchive that the GC keeps alive. Here a provider returns a
//     unique_ptr and the *caller* owns what it gets back (TBulkData holds it for the length of one read),
//     so GetPayload returns an owning pointer rather than a borrowed one. The dictionary itself is
//     shared_ptr-held so clones share it, matching C#'s "carry over the payloads dict to the clone".
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../Readers/FArchive.h"
#include "../IPackage.h"
#include "../../Objects/UObject/FName.h"
#include "../Utils/PayloadType.h"

namespace CUE4Parse::UE4::Assets::Objects { struct FByteBulkDataHeader; }

namespace CUE4Parse::UE4::Assets::Readers
{
    using CUE4Parse::UE4::Readers::FArchive;
    using CUE4Parse::UE4::Readers::ESeekOrigin;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Assets::Utils::PayloadType;

    class FAssetArchive : public FArchive
    {
    public:
        // C#'s Func<FByteBulkDataHeader?, FArchive?>: given the header being resolved (null when the caller
        // wants the whole payload file), hand back the raw payload archive, or null if it cannot be opened.
        using RawPayloadProvider = std::function<std::unique_ptr<FArchive>(const Objects::FByteBulkDataHeader*)>;
        // The stored form, after AddPayload has wrapped the raw archive in an FAssetArchive.
        using PayloadProvider = std::function<std::unique_ptr<FAssetArchive>(const Objects::FByteBulkDataHeader*)>;
        using PayloadMap = std::map<PayloadType, PayloadProvider>;

        IPackage* Owner = nullptr;
        int AbsoluteOffset = 0;

        FAssetArchive(FArchive& baseArchive, IPackage* owner, int absoluteOffset = 0,
                      std::shared_ptr<PayloadMap> payloads = nullptr)
            : FArchive(baseArchive.Versions), Owner(owner), AbsoluteOffset(absoluteOffset),
              _payloads(payloads ? std::move(payloads) : std::make_shared<PayloadMap>()),
              _baseArchive(&baseArchive)
        {
            Length = baseArchive.Length;
            Position = baseArchive.Position;
        }

        // C#'s SetBaseArchive.
        void SetBaseArchive(FArchive& newArchive) { _baseArchive = &newArchive; _ownedBase.reset(); }

        // Builds an archive that OWNS its base reader. C# never needs this — the GC keeps the base alive
        // behind the reference — but the bulk-data layer creates payload archives over readers it opened
        // itself, and those have to outlive the call that made them.
        static std::unique_ptr<FAssetArchive> CreateOwning(std::unique_ptr<FArchive> baseArchive, IPackage* owner,
                                                          int absoluteOffset = 0)
        {
            return std::unique_ptr<FAssetArchive>(
                new FAssetArchive(std::move(baseArchive), owner, absoluteOffset, nullptr));
        }

        // Returns null instead of throwing when the payload is absent (C#'s TryGetPayload).
        std::unique_ptr<FAssetArchive> TryGetPayload(PayloadType type,
                                                    const Objects::FByteBulkDataHeader* header = nullptr) const;
        // Throws a ParserException when the payload is absent, as C# does.
        std::unique_ptr<FAssetArchive> GetPayload(PayloadType type,
                                                 const Objects::FByteBulkDataHeader* header = nullptr) const;
        void AddPayload(PayloadType type, int absoluteOffset, RawPayloadProvider payload);
        // The fixed-archive form. C# hands the same instance back every time; this port hands back a clone,
        // since the caller owns what GetPayload returns.
        void AddPayload(PayloadType type, std::shared_ptr<FAssetArchive> payload);

        bool HasUnversionedProperties() const { return Owner && Owner->HasFlags(CUE4Parse::UE4::Objects::UObject::PKG_UnversionedProperties); }
        bool IsFilterEditorOnly() const { return Owner && Owner->HasFlags(CUE4Parse::UE4::Objects::UObject::PKG_FilterEditorOnly); }
        bool IsLoadingFromCookedPackage() const { return Owner && Owner->HasFlags(CUE4Parse::UE4::Objects::UObject::PKG_Cooked); }

        int Read(uint8_t* buffer, int offset, int count) override;
        int64_t Seek(int64_t offset, ESeekOrigin origin) override;
        int64_t SeekAbsolute(int64_t offset, ESeekOrigin origin);
        int64_t AbsolutePosition() const { return AbsoluteOffset + Position; }
        bool CanSeek() const override { return _baseArchive->CanSeek(); }
        const std::string& Name() const override { return _baseArchive->Name(); }
        std::unique_ptr<FArchive> Clone() const override;

        FName ReadFName() override;
        // Peeks whether the next 8 bytes look like a valid (nameIndex, extraIndex) FName pair.
        bool TestReadFName();

        // Fast hiding overloads (read directly from the base store). `using` keeps the byte-source Read
        // and the base Read<T>/ReadArray<T> templates visible (declaring Read here hides them by name).
        using FArchive::Read;
        template <typename T>
        T Read()
        {
            static_assert(std::is_trivially_copyable_v<T>, "FAssetArchive::Read<T> requires a trivially copyable T");
            T result{};
            Read(reinterpret_cast<uint8_t*>(&result), 0, static_cast<int>(sizeof(T)));
            return result;
        }

        template <typename T>
        std::vector<T> ReadArray(int length)
        {
            static_assert(std::is_trivially_copyable_v<T>, "FAssetArchive::ReadArray<T> requires a trivially copyable T");
            const int size = length * static_cast<int>(sizeof(T));
            CheckReadSize(size);
            std::vector<T> result(static_cast<size_t>(length));
            if (length > 0) Read(reinterpret_cast<uint8_t*>(result.data()), 0, size);
            return result;
        }

    private:
        // Shared with clones, exactly as C# shares the dictionary reference: payloads are added during
        // package initialization, never during object serialization, so sharing costs nothing.
        std::shared_ptr<PayloadMap> _payloads;
        // Non-owning pointer to the base archive (the normal case, matching C#'s reference). Clones own
        // their cloned base via _ownedBase.
        FArchive* _baseArchive;
        std::unique_ptr<FArchive> _ownedBase;

        FAssetArchive(std::unique_ptr<FArchive> ownedBase, IPackage* owner, int absoluteOffset,
                      std::shared_ptr<PayloadMap> payloads)
            : FArchive(ownedBase->Versions), Owner(owner), AbsoluteOffset(absoluteOffset),
              _payloads(payloads ? std::move(payloads) : std::make_shared<PayloadMap>()),
              _baseArchive(ownedBase.get()), _ownedBase(std::move(ownedBase))
        {
            Length = _baseArchive->Length;
            Position = _baseArchive->Position;
        }
    };
}
