// Ported from CUE4Parse/UE4/Assets/AbstractUePackage.cs (the Mappings => Provider?.MappingsForGame default).
#include "IPackage.h"

#include "../../FileProvider/IFileProvider.h"

namespace CUE4Parse::UE4::Assets
{
    const CUE4Parse::MappingsProvider::TypeMappings* IPackage::Mappings() const
    {
        auto* provider = GetProvider();
        return provider != nullptr ? provider->MappingsForGame() : nullptr;
    }
}
