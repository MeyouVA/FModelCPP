// Ported from CUE4Parse/UE4/Objects/UObject/UStruct.cs.
#include "UStruct.h"

#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    void UStruct::Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UField::Deserialize(Ar, validPos);

        SuperStruct = FPackageIndex(Ar);

        // Modern (RemoveUField_Next present): Children is a counted FPackageIndex array.
        const int32_t childCount = Ar.Read<int32_t>();
        Children.reserve(static_cast<size_t>(childCount));
        for (int32_t i = 0; i < childCount; i++)
            Children.emplace_back(Ar);

        // Modern (FCoreObjectVersion >= FProperties): read ChildProperties (the FProperties system).
        const int32_t propCount = Ar.Read<int32_t>();
        ChildProperties.reserve(static_cast<size_t>(propCount));
        for (int32_t i = 0; i < propCount; i++)
        {
            const FName propertyTypeName = Ar.ReadFName();
            auto prop = FField::Construct(propertyTypeName);
            prop->Deserialize(Ar);
            ChildProperties.push_back(std::move(prop));
        }

        const int32_t bytecodeBufferSize = Ar.Read<int32_t>();
        (void)bytecodeBufferSize;
        const int32_t serializedScriptSize = Ar.Read<int32_t>();
        // ReadScriptData isn't ported (C# default false) -> skip the serialized bytecode block.
        Ar.Position += serializedScriptSize;
    }

    FField* UStruct::GetProperty(const FName& name) const
    {
        for (const auto& item : ChildProperties)
        {
            if (item->Name.Text() == name.Text())
                return item.get();
        }
        return nullptr;
    }
}
