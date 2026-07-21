// Ported from CUE4Parse/UE4/Objects/UObject/FSoftObjectPath.cs
// A soft (by-name) reference to a top-level object in a package: /package/path.assetname plus an optional
// sub-path after the ':'. Read from an FAssetArchive.
//
// Deliberate differences from C#:
//   * Only the classic (post-ADDED_SOFT_OBJECT_PATH, non-UE5-list, non-game-specific) read path is ported:
//     AssetPathName via ReadFName and SubPathString via ReadFString. The deferred branches are the pre-
//     ADDED_SOFT_OBJECT_PATH string form, the UE5 ADD_SOFTOBJECTPATH_LIST index-into-package-table form, the
//     FSOFTOBJECTPATH_REMOVE_ASSET_PATH_FNAMES FTopLevelAssetPath form, the FFortniteMainBranchObjectVersion
//     UTF-8 sub-path gate, and the AshesOfCreation / OuterWorlds2 / DragonQuestXI game readers. TODO.
//   * The Load/TryLoad family is ported (sync only). C#'s LoadAsync/TryLoadAsync variants have no C++ Task
//     equivalent and are omitted. C#'s Load(provider) throws when the package/object is missing (the provider
//     throws); this port's IFileProvider::LoadPackageObject returns null instead, so Load(provider) returns
//     null on a miss (the typed Load<T> still throws on a wrong-type / null result, matching C#).
#pragma once

#include <string>

#include "../../IUStruct.h"
#include "FName.h"
#include "../../Assets/Exports/UObject.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets { class IPackage; }
namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }
namespace CUE4Parse::FileProvider { class IFileProvider; }

namespace CUE4Parse::UE4::Objects::UObject
{
    struct FSoftObjectPath : public UE4::IUStruct
    {
        using UExport = Assets::Exports::UObject;

        // Asset path, path to a top level object in a package. This is /package/path.assetname
        FName AssetPathName;
        // Optional FString for subobject within an asset. This is the sub path after the ':'
        std::string SubPathString;

        Assets::IPackage* Owner = nullptr;

        FSoftObjectPath() = default;
        explicit FSoftObjectPath(Assets::Readers::FAssetArchive& Ar);
        FSoftObjectPath(FName assetPathName, std::string subPathString, Assets::IPackage* owner = nullptr)
            : AssetPathName(std::move(assetPathName)), SubPathString(std::move(subPathString)), Owner(owner) {}

        std::string ToString() const
        {
            if (SubPathString.empty())
                return AssetPathName.IsNone() ? "" : AssetPathName.Text();
            return AssetPathName.Text() + ":" + SubPathString;
        }

        // --- Loading (needs an IFileProvider). Defined in the .cpp; typed overloads inline below. ---
        UExport* Load(FileProvider::IFileProvider& provider) const;
        bool TryLoad(FileProvider::IFileProvider& provider, UExport*& outExport) const;
        // These resolve the provider from Owner->GetProvider(). Load() throws if the package had no provider.
        UExport* Load() const;
        bool TryLoad(UExport*& outExport) const;

        template <typename T>
        T* Load(FileProvider::IFileProvider& provider) const
        {
            T* cast = dynamic_cast<T*>(Load(provider));
            if (cast == nullptr)
                throw Exceptions::ParserException("Loaded SoftObjectProperty but it was of wrong type");
            return cast;
        }
        template <typename T>
        bool TryLoad(FileProvider::IFileProvider& provider, T*& outExport) const
        {
            UExport* generic = nullptr;
            if (!TryLoad(provider, generic)) { outExport = nullptr; return false; }
            outExport = dynamic_cast<T*>(generic);
            return outExport != nullptr;
        }
        template <typename T>
        T* Load() const
        {
            T* cast = dynamic_cast<T*>(Load());
            if (cast == nullptr)
                throw Exceptions::ParserException("Loaded SoftObjectProperty but it was of wrong type");
            return cast;
        }
        template <typename T>
        bool TryLoad(T*& outExport) const
        {
            UExport* generic = nullptr;
            if (!TryLoad(generic)) { outExport = nullptr; return false; }
            outExport = dynamic_cast<T*>(generic);
            return outExport != nullptr;
        }

    private:
        // Walks SubPathString ('.'-separated export names) from `asset`; identity when SubPathString is empty.
        bool TryResolveSubObject(UExport* asset, UExport*& outExport) const;
    };
}
