// Ported from CUE4Parse/UE4/Assets/Exports/Engine/UCurveTable.cs.
#include "UCurveTable.h"

#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Engine/Curves/RealCurve.h"
#include "../../../Objects/Engine/Curves/SimpleCurve.h"
#include "../../../Objects/Engine/Curves/RichCurve.h"

namespace CUE4Parse::UE4::Assets::Exports::Engine
{
    using Readers::FAssetArchive;
    namespace Curves = CUE4Parse::UE4::Objects::Engine::Curves;

    void UCurveTable::Deserialize(FAssetArchive& Ar, int64_t validPos)
    {
        UObject::Deserialize(Ar, validPos);

        const int32_t numRows = Ar.Read<int32_t>();

        // assume-modern: FFortniteMainBranchObjectVersion >= ShrinkCurveTableSize, so the mode is a serialized
        // byte. The pre-Shrink path inferred it from numRows (numRows > 0 ? RichCurves : Empty). TODO: gate on
        // the real custom version.
        CurveTableMode = Ar.Read<ECurveTableMode>();

        RowMap.reserve(numRows);
        for (int32_t i = 0; i < numRows; i++)
        {
            FName rowName = Ar.ReadFName();
            // Curve row structs: SimpleCurves -> "SimpleCurve", RichCurves -> "RichCurve", Empty -> "". The name
            // is only consulted on the (deferred) unversioned path; the tagged stream reads the same either way.
            std::string rowStruct = CurveTableMode == ECurveTableMode::SimpleCurves ? "SimpleCurve"
                                  : CurveTableMode == ECurveTableMode::RichCurves   ? "RichCurve"
                                                                                    : "";
            RowMap.emplace_back(std::move(rowName), FStructFallback(Ar, rowStruct));
        }
    }

    std::unique_ptr<Curves::FRealCurve> UCurveTable::FindCurve(const FName& rowName) const
    {
        if (rowName.IsNone())
            return nullptr; // C# logs "NAME_None is invalid row name".

        const FStructFallback* found = nullptr;
        for (const auto& row : RowMap)
        {
            if (row.first == rowName)
            {
                found = &row.second;
                break;
            }
        }
        if (found == nullptr)
            return nullptr; // C# logs "Row not found".

        switch (CurveTableMode)
        {
            case ECurveTableMode::SimpleCurves:
                return std::make_unique<Curves::FSimpleCurve>(*found);
            case ECurveTableMode::RichCurves:
                return std::make_unique<Curves::FRichCurve>(*found);
            default:
                return nullptr;
        }
    }

    bool UCurveTable::TryFindCurve(const FName& rowName,
                                   std::unique_ptr<Curves::FRealCurve>& outCurve) const
    {
        outCurve = FindCurve(rowName);
        return outCurve != nullptr;
    }
}
