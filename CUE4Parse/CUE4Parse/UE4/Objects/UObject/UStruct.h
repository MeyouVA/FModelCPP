// Ported from CUE4Parse/UE4/Objects/UObject/UStruct.cs
// UStruct: a reflection container with a SuperStruct, Children (UField package indices), ChildProperties
// (the FProperties system), and (skipped) script bytecode.
//
// Deliberate differences from C#:
//   * Custom-version gates (FFrameworkObjectVersion.RemoveUField_Next, FCoreObjectVersion.FProperties) are not
//     ported; the modern outcome is assumed — Children is a counted array and ChildProperties are always read.
//   * ScriptBytecode is not parsed: FKismetArchive / KismetExpression aren't ported, and the provider's
//     ReadScriptData flag isn't either (defaults false in C#), so the serialized script block is skipped. TODO.
//   * `[SkipObjectRegistration]` in C# — UStruct is intentionally NOT registered in ObjectTypeRegistry.
#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "UField.h"
#include "FField.h"
#include "ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class UStruct : public UField
    {
    public:
        FPackageIndex SuperStruct;
        std::vector<FPackageIndex> Children;
        std::vector<std::unique_ptr<FField>> ChildProperties;
        // ScriptBytecode is intentionally not stored (see header note).

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;

        // ignore inner properties and return the main one (C# UStruct.GetProperty)
        FField* GetProperty(const FName& name) const;
    };
}
