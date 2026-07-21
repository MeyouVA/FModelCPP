// Ported from CUE4Parse/UE4/Objects/Engine/UBlueprintGeneratedClass.cs
// A UBlueprintGeneratedClass export: the UClass a Blueprint compiles to, plus a few Blueprint-specific
// references and (in cooked editor builds) an editor tag map.
//
// Deliberate differences from C#:
//   * C# reads NumReplicatedProperties / the component & timeline arrays / the construction-script indices
//     via the reflection accessor GetOrDefault<T>(nameof(...)), which pulls them out of the already-parsed
//     tagged Properties. That accessor is not ported (see UObject.h), so Deserialize populates these fields
//     by scanning the base Properties list directly (mirroring UDataTable's RowStruct extraction). They have
//     no downstream consumer yet; they exist for structural fidelity and to be testable.
//   * EditorTags is the only genuinely custom-serialized member (it is not a tagged property). It is an
//     insertion-ordered vector<pair<FName,string>> rather than a Dictionary (FName has no hash), empty when
//     absent. Its version gate (FFortniteMainBranchObjectVersion) is assumed modern — see the .cpp.
//   * The commented-out FBPComponentClassOverride / FFieldNotificationId arrays and the WorldofJadeDynasty
//     quirk are omitted (as in C#, or with a TODO). WriteJson is omitted.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../UObject/UClass.h"
#include "../UObject/ObjectResource.h"
#include "../UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::Engine
{
    using CUE4Parse::UE4::Objects::UObject::UClass;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UBlueprintGeneratedClass : public UClass
    {
    public:
        // Cached views of tagged properties (C# reads these via GetOrDefault; populated from Properties here).
        int32_t NumReplicatedProperties = 0;
        std::vector<FPackageIndex> DynamicBindingObjects;
        std::vector<FPackageIndex> ComponentTemplates;
        std::vector<FPackageIndex> Timelines;
        std::optional<FPackageIndex> SimpleConstructionScript;
        std::optional<FPackageIndex> InheritableComponentHandler;
        std::optional<FPackageIndex> UberGraphFunction;

        // Custom-serialized cooked editor tag map (not a tagged property). Empty when absent.
        std::vector<std::pair<FName, std::string>> EditorTags;

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
