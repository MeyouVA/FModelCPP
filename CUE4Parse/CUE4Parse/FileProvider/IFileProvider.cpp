// Ported from CUE4Parse/FileProvider/AbstractFileProvider.cs (the GetPathName + LoadPackageObject slice).
#include "IFileProvider.h"

#include "../UE4/Assets/IPackage.h"
#include "../UE4/Assets/Exports/UObject.h"

namespace CUE4Parse::FileProvider
{
    using UE4::Assets::Exports::UObject;

    UObject* IFileProvider::LoadPackageObject(const std::string& path)
    {
        // C#'s GetPathName: split on the last '.', otherwise the object name is the segment after the last
        // '/' and the whole string is the package path.
        std::string packagePath = path;
        std::string objectName;
        const auto dot = path.find_last_of('.');
        if (dot == std::string::npos)
        {
            const auto slash = path.find_last_of('/');
            objectName = slash == std::string::npos ? path : path.substr(slash + 1);
        }
        else
        {
            objectName = path.substr(dot + 1);
            packagePath = path.substr(0, dot);
        }
        if (packagePath.empty() || objectName.empty())
            return nullptr;

        UE4::Assets::IPackage* pkg = TryLoadPackage(packagePath);
        if (pkg == nullptr)
            return nullptr;
        const int index = pkg->GetExportIndex(objectName);
        if (index < 0)
            return nullptr;
        return pkg->GetExportObject(index);
    }
}
