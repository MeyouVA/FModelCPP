// Ported from CUE4Parse/UE4/Objects/UObject/FFieldPath.cs
// A path to a UField (a chain of FNames) plus its resolved owner UStruct (an FPackageIndex).
//
// Deliberate differences from C#:
//   * The FKismetArchive ctor is omitted until that reader is ported.
//   * The ResolvedOwner read is gated in C# on FFortniteMainBranchObjectVersion / FReleaseObjectVersion custom
//     versions (FFieldPathOwnerSerialization). Custom versions aren't ported, so the port assumes modern assets
//     (owner serialization present) and always reads it. TODO: gate once the custom-version providers exist.
//   * The reflection/JSON WriteJson resolution (TryLoad<UField> / GetProperty) is omitted with object loading.
#pragma once

#include <string>
#include <vector>

#include "FName.h"
#include "ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class FFieldPath
    {
    public:
        std::vector<FName> Path;
        FPackageIndex ResolvedOwner; // UStruct

        FFieldPath() = default;
        explicit FFieldPath(Assets::Readers::FAssetArchive& Ar);

        std::string ToString() const { return Path.empty() ? std::string() : Path[0].Text(); }
    };
}
