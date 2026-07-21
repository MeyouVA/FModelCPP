// Ported from CUE4Parse/UE4/Objects/UObject/UClass.cs
// A UClass export = a UStruct plus class flags, the function map, implemented interfaces and the CDO index.
//
// Deliberate differences from C#:
//   * `[SkipObjectRegistration]` — UClass is intentionally NOT registered (a raw "Class" export falls back to a
//     base UObject, as in C#; concrete blueprint classes come via the un-ported UBlueprintGeneratedClass).
//   * UClass.ConstructObject(flags) and DecompileBlueprintToPseudo (the Kismet/BlueprintDecompiler layer) are
//     omitted. TODO.
//   * FuncMap is an insertion-ordered vector<pair<FName, FPackageIndex>> (FName has no hash), mirroring UDataTable.
//   * Game-specific quirks (AWayOut / StarWarsJedi / AshesOfCreation / Borderlands4) are omitted. TODO.
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "UStruct.h"
#include "EClassFlags.h"
#include "ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class UClass : public UStruct
    {
    public:
        struct FImplementedInterface
        {
            FPackageIndex Class;      // the interface class
            int32_t PointerOffset = 0; // pointer offset of the interface's vtable
            bool bImplementedByK2 = false;

            explicit FImplementedInterface(Assets::Readers::FAssetArchive& Ar);
        };

        bool bCooked = false;
        EClassFlags ClassFlags = CLASS_None;
        FPackageIndex ClassWithin;
        FPackageIndex ClassGeneratedBy;
        FName ClassConfigName;
        FPackageIndex ClassDefaultObject;
        std::vector<std::pair<FName, FPackageIndex>> FuncMap;
        std::vector<FImplementedInterface> Interfaces;

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
