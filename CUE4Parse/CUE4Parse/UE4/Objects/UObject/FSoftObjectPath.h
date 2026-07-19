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
//   * The Load/TryLoad family (which needs IFileProvider) is deferred with the object-loading layer.
#pragma once

#include <string>

#include "../../IUStruct.h"
#include "FName.h"

namespace CUE4Parse::UE4::Assets { class IPackage; }
namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    struct FSoftObjectPath : public UE4::IUStruct
    {
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
    };
}
