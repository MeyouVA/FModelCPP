// Ported from CUE4Parse/UE4/Objects/Engine/UBlueprintGeneratedClass.cs
#include "UBlueprintGeneratedClass.h"

#include "../../Assets/Readers/FAssetArchive.h"
#include "../../Assets/Objects/FPropertyTag.h"
#include "../../Assets/Objects/Properties/IntProperty.h"
#include "../../Assets/Objects/Properties/ObjectProperty.h"
#include "../../Assets/Objects/Properties/ArrayProperty.h"

namespace CUE4Parse::UE4::Objects::Engine
{
    using Assets::Readers::FAssetArchive;
    using Assets::Objects::FPropertyTag;
    using Assets::Objects::Properties::IntProperty;
    using Assets::Objects::Properties::ObjectProperty;
    using Assets::Objects::Properties::ArrayProperty;

    namespace
    {
        // C#'s GetOrDefault<T>(name) reads a tagged property by name; without the reflection accessor we scan
        // the parsed Properties list (the same source C#'s GetOrDefault ultimately reads from).
        const FPropertyTag* FindTag(const std::vector<FPropertyTag>& props, const char* name)
        {
            for (const auto& tag : props)
                if (tag.Name.Text() == name)
                    return &tag;
            return nullptr;
        }

        std::optional<FPackageIndex> ReadIndex(const FPropertyTag* tag)
        {
            if (tag != nullptr)
                if (auto* op = dynamic_cast<const ObjectProperty*>(tag->Tag.get()))
                    return op->Value;
            return std::nullopt;
        }

        void ReadIndexArray(const FPropertyTag* tag, std::vector<FPackageIndex>& out)
        {
            if (tag == nullptr) return;
            if (auto* ap = dynamic_cast<const ArrayProperty*>(tag->Tag.get()))
                for (const auto& elem : ap->Value.Properties)
                    if (auto* op = dynamic_cast<const ObjectProperty*>(elem.get()))
                        out.push_back(op->Value);
        }
    }

    void UBlueprintGeneratedClass::Deserialize(FAssetArchive& Ar, int64_t validPos)
    {
        UClass::Deserialize(Ar, validPos);

        // Tagged-property views (GetOrDefault in C#). None of these advance the archive; they read Properties.
        if (auto* tag = FindTag(Properties, "NumReplicatedProperties"))
            if (auto* ip = dynamic_cast<const IntProperty*>(tag->Tag.get()))
                NumReplicatedProperties = ip->Value;
        ReadIndexArray(FindTag(Properties, "DynamicBindingObjects"), DynamicBindingObjects);
        ReadIndexArray(FindTag(Properties, "ComponentTemplates"), ComponentTemplates);
        ReadIndexArray(FindTag(Properties, "Timelines"), Timelines);
        SimpleConstructionScript = ReadIndex(FindTag(Properties, "SimpleConstructionScript"));
        InheritableComponentHandler = ReadIndex(FindTag(Properties, "InheritableComponentHandler"));
        UberGraphFunction = ReadIndex(FindTag(Properties, "UberGraphFunction"));

        // assume-modern: FFortniteMainBranchObjectVersion.Get(Ar) >= BPGCCookedEditorTags. The custom-version
        // provider is not ported, so the gate is hardcoded to its modern (true) outcome. TODO: port the
        // provider if a pre-BPGCCookedEditorTags asset must be read. The WorldofJadeDynasty +24 quirk is omitted.
        if (validPos - Ar.Position > 4)
        {
            // C#: Ar.ReadMap(Ar.ReadFName, Ar.ReadFString) — an int32 count then that many (FName, FString) pairs.
            const int32_t count = Ar.Read<int32_t>();
            EditorTags.reserve(static_cast<size_t>(count));
            for (int32_t i = 0; i < count; i++)
            {
                FName key = Ar.ReadFName();
                std::string value = Ar.ReadFString();
                EditorTags.emplace_back(std::move(key), std::move(value));
            }
        }
    }
}
