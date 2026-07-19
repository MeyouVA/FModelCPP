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
//   * Object resolution/loading (ReadObject<T>/ReadUObject) and the bulk-data payload subsystem
//     (GetPayload/AddPayload/TryGetPayload, PayloadType, FByteBulkDataHeader) are deferred with the asset-
//     export layer. TODO: bring them over then.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Readers/FArchive.h"
#include "../IPackage.h"
#include "../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Readers
{
    using CUE4Parse::UE4::Readers::FArchive;
    using CUE4Parse::UE4::Readers::ESeekOrigin;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FAssetArchive : public FArchive
    {
    public:
        IPackage* Owner = nullptr;
        int AbsoluteOffset = 0;

        FAssetArchive(FArchive& baseArchive, IPackage* owner, int absoluteOffset = 0)
            : FArchive(baseArchive.Versions), Owner(owner), AbsoluteOffset(absoluteOffset), _baseArchive(&baseArchive)
        {
            Length = baseArchive.Length;
            Position = baseArchive.Position;
        }

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
        // Non-owning pointer to the base archive (the normal case, matching C#'s reference). Clones own
        // their cloned base via _ownedBase.
        FArchive* _baseArchive;
        std::unique_ptr<FArchive> _ownedBase;

        FAssetArchive(std::unique_ptr<FArchive> ownedBase, IPackage* owner, int absoluteOffset)
            : FArchive(ownedBase->Versions), Owner(owner), AbsoluteOffset(absoluteOffset), _baseArchive(ownedBase.get()), _ownedBase(std::move(ownedBase))
        {
            Length = _baseArchive->Length;
            Position = _baseArchive->Position;
        }
    };
}
