// Ported from CUE4Parse/UE4/Objects/UObject/UClass.cs (Deserialize only).
#include "UClass.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using namespace CUE4Parse::UE4::Versions;

    UClass::FImplementedInterface::FImplementedInterface(Assets::Readers::FAssetArchive& Ar)
    {
        Class = FPackageIndex(Ar);
        PointerOffset = Ar.Read<int32_t>();
        bImplementedByK2 = Ar.ReadBoolean();
    }

    void UClass::Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UStruct::Deserialize(Ar, validPos);

        const int32_t funcCount = Ar.Read<int32_t>();
        FuncMap.reserve(static_cast<size_t>(funcCount));
        for (int32_t i = 0; i < funcCount; i++)
        {
            FName key = Ar.ReadFName();
            FuncMap.emplace_back(std::move(key), FPackageIndex(Ar));
        }

        ClassFlags = Ar.Read<EClassFlags>();
        ClassWithin = FPackageIndex(Ar);
        ClassConfigName = Ar.ReadFName();
        ClassGeneratedBy = FPackageIndex(Ar);

        const int32_t ifaceCount = Ar.Read<int32_t>();
        Interfaces.reserve(static_cast<size_t>(ifaceCount));
        for (int32_t i = 0; i < ifaceCount; i++)
            Interfaces.emplace_back(Ar);

        (void)Ar.ReadBoolean();
        (void)Ar.ReadFName();

        if (Ar.Ver() >= EUnrealEngineObjectUE4Version::ADD_COOKED_TO_UCLASS)
            bCooked = Ar.ReadBoolean();

        ClassDefaultObject = FPackageIndex(Ar);
    }
}
