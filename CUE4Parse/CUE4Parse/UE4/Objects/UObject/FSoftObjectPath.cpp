// Ported from CUE4Parse/UE4/Objects/UObject/FSoftObjectPath.cs (reading ctor + sync Load/TryLoad family).
#include "FSoftObjectPath.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Assets/IPackage.h"
#include "../../../FileProvider/IFileProvider.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    FSoftObjectPath::FSoftObjectPath(Assets::Readers::FAssetArchive& Ar)
    {
        // Classic path. See the header note for the deferred version/game branches.
        AssetPathName = Ar.ReadFName();
        SubPathString = Ar.ReadFString();
        Owner = Ar.Owner;
    }

    FSoftObjectPath::UExport* FSoftObjectPath::Load(FileProvider::IFileProvider& provider) const
    {
        return provider.LoadPackageObject(AssetPathName.Text());
    }

    bool FSoftObjectPath::TryLoad(FileProvider::IFileProvider& provider, UExport*& outExport) const
    {
        UExport* asset = provider.LoadPackageObject(AssetPathName.Text());
        if (asset == nullptr)
        {
            outExport = nullptr;
            return false;
        }
        return TryResolveSubObject(asset, outExport);
    }

    FSoftObjectPath::UExport* FSoftObjectPath::Load() const
    {
        FileProvider::IFileProvider* provider = Owner != nullptr ? Owner->GetProvider() : nullptr;
        if (provider == nullptr)
            throw Exceptions::ParserException("Package was loaded without a IFileProvider");
        return Load(*provider);
    }

    bool FSoftObjectPath::TryLoad(UExport*& outExport) const
    {
        FileProvider::IFileProvider* provider = Owner != nullptr ? Owner->GetProvider() : nullptr;
        if (provider == nullptr || AssetPathName.IsNone() || AssetPathName.Text().empty())
        {
            outExport = nullptr;
            return false;
        }
        return TryLoad(*provider, outExport);
    }

    bool FSoftObjectPath::TryResolveSubObject(UExport* asset, UExport*& outExport) const
    {
        if (SubPathString.empty())
        {
            outExport = asset;
            return true;
        }

        UExport* current = asset;
        size_t start = 0;
        while (true)
        {
            const size_t dot = SubPathString.find('.', start);
            const std::string part = SubPathString.substr(
                start, dot == std::string::npos ? std::string::npos : dot - start);

            if (current->Owner == nullptr)
            {
                outExport = nullptr;
                return false;
            }
            UExport* found = current->Owner->GetExportOrNull(part);
            if (found == nullptr)
            {
                // C# logs "Could not find subobject ...".
                outExport = nullptr;
                return false;
            }
            current = found;

            if (dot == std::string::npos) break;
            start = dot + 1;
        }

        outExport = current;
        return true;
    }
}
