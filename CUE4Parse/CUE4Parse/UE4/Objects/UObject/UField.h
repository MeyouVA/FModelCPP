// Ported from CUE4Parse/UE4/Objects/UObject/UField.cs
// UField: base of the reflection object types (UStruct/UEnum/...). Holds the legacy `Next` linked-list pointer.
//
// Deliberate difference from C#: the `Next` field is gated on FFrameworkObjectVersion < RemoveUField_Next, an
// un-ported custom version. Assuming modern assets (RemoveUField_Next present), Next is never serialized. TODO.
#pragma once

#include <cstdint>
#include <optional>

#include "../../Assets/Exports/UObject.h"
#include "ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class UField : public CUE4Parse::UE4::Assets::Exports::UObject
    {
    public:
        std::optional<FPackageIndex> Next; // UField (legacy, absent on modern assets)

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
