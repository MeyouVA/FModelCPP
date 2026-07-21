// Ported from CUE4Parse/UE4/Objects/UObject/UEnum.cs (modern layout only).
#include "UEnum.h"

#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    void UEnum::Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UField::Deserialize(Ar, validPos);

        // Modern (FCoreObjectVersion >= EnumProperties): Names is a counted array of (FName, int64).
        const int32_t count = Ar.Read<int32_t>();
        Names.reserve(static_cast<size_t>(count));
        for (int32_t i = 0; i < count; i++)
        {
            FName name = Ar.ReadFName();
            const int64_t value = Ar.Read<int64_t>();
            Names.emplace_back(std::move(name), value);
        }

        // Modern (Ar.Ver >= ENUM_CLASS_SUPPORT): CppForm is a single byte.
        CppForm = Ar.Read<ECppForm>();
        // Modern (FFortniteMainBranchObjectVersion >= EnumUnderlyingType): the underlying type follows.
        UnderlyingType = Ar.Read<EUnderlyingType>();
    }
}
