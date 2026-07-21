// Ported from CUE4Parse/UE4/Assets/Exports/Engine/UCurveTable.cs
// A UCurveTable export: a map of row name -> a curve stored as a bag of tagged properties (FStructFallback).
// The CurveTableMode selects which curve struct each row is (SimpleCurve / RichCurve / none), which in C# only
// affects the struct-type *name* handed to FStructFallback (and the FindCurve wrapper).
//
// Deliberate differences from C# (mirrors the UDataTable port):
//   * RowMap is an insertion-ordered vector<pair<FName, FStructFallback>> rather than a Dictionary: FName has
//     no hash/ordering and C#'s Dictionary iterates in ~insertion order.
//   * FindCurve/TryFindCurve are ported (Session 25) atop the Objects/Engine/Curves eval subsystem: they wrap a
//     row's FStructFallback in an FSimpleCurve (SimpleCurves mode) or FRichCurve (RichCurves mode). C# logs +
//     returns null for NAME_None / missing rows; this port has no logging, so it simply returns null / false.
//   * The bUpgradingCurveTable branch (FFortniteMainBranchObjectVersion < ShrinkCurveTableSize, which infers the
//     mode from numRows instead of reading a byte) is assume-modern: we always read the serialized mode byte.
//     TODO: gate on the real custom version.
//   * WriteJson is omitted.
#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "ECurveTableMode.h"
#include "../UObject.h"
#include "../../Objects/FStructFallback.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }
namespace CUE4Parse::UE4::Objects::Engine::Curves { class FRealCurve; }

namespace CUE4Parse::UE4::Assets::Exports::Engine
{
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;

    class UCurveTable : public UObject
    {
    public:
        std::vector<std::pair<FName, FStructFallback>> RowMap;
        ECurveTableMode CurveTableMode = ECurveTableMode::Empty;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;

        // Wraps the named row's FStructFallback in the concrete curve for the current CurveTableMode. Returns
        // null if the row is NAME_None / not found / the table is Empty (C# also logs a warning in those cases).
        std::unique_ptr<CUE4Parse::UE4::Objects::Engine::Curves::FRealCurve> FindCurve(const FName& rowName) const;
        bool TryFindCurve(const FName& rowName,
                          std::unique_ptr<CUE4Parse::UE4::Objects::Engine::Curves::FRealCurve>& outCurve) const;
    };
}
