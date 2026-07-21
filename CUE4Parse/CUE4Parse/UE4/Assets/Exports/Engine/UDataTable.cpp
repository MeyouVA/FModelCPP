// Ported from CUE4Parse/UE4/Assets/Exports/Engine/UDataTable.cs.
#include "UDataTable.h"

#include "../../Readers/FAssetArchive.h"
#include "../../Objects/Properties/ObjectProperty.h"

namespace CUE4Parse::UE4::Assets::Exports::Engine
{
    using Readers::FAssetArchive;
    using CUE4Parse::UE4::Assets::Objects::Properties::ObjectProperty;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    void UDataTable::Deserialize(FAssetArchive& Ar, int64_t validPos)
    {
        UObject::Deserialize(Ar, validPos);

        // C#: GetOrDefault<FPackageIndex?>("RowStruct"), then ptr.TryLoad<UStruct> to drive parsing. Object
        // loading of the row struct is deferred, so we only recover its name (ptr.Name) from the tagged
        // "RowStruct" ObjectProperty and fall through to the by-name FStructFallback path below.
        if (!RowStructName.has_value() || RowStructName->empty())
        {
            const FPackageIndex* rowStruct = nullptr;
            for (const auto& tag : Properties)
            {
                if (tag.Name.Text() == "RowStruct")
                {
                    if (auto* op = dynamic_cast<const ObjectProperty*>(tag.Tag.get()))
                        rowStruct = &op->Value;
                    break;
                }
            }

            if (rowStruct != nullptr && !rowStruct->IsNull())
            {
                RowStructName = rowStruct->Name();
            }
            else
            {
                // C# logs "Can't find or load RowStruct type to serialize DataTable" and returns.
                return;
            }
        }

        const int32_t numRows = Ar.Read<int32_t>();
        RowMap.reserve(numRows);
        for (int32_t i = 0; i < numRows; i++)
        {
            FName rowName = Ar.ReadFName();
            RowMap.emplace_back(std::move(rowName), FStructFallback(Ar, RowStructName));
        }
    }
}
