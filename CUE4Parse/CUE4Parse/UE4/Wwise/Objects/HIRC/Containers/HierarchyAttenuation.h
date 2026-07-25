// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyAttenuation.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../AkConversionTable.h"
#include "../../AkRTPC.h"
#include "../AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    class ConeParams
    {
    public:
        float fInsideDegrees = 0;
        float fOutsideDegrees = 0;
        float fOutsideVolume = 0;
        float LoPass = 0;
        float HiPass = 0;

        ConeParams() = default;

        explicit ConeParams(FWwiseArchive& Ar)
        {
            fInsideDegrees = Ar.Read<float>();
            fOutsideDegrees = Ar.Read<float>();
            fOutsideVolume = Ar.Read<float>();
            LoPass = Ar.Read<float>();
            if (Ar.Version > 89)
            {
                HiPass = Ar.Read<float>();
            }
        }
    };

    // CAkAttenuation
    class HierarchyAttenuation : public AbstractHierarchy
    {
    public:
        bool IsHeightSpreadEnabled = false;
        bool IsConeEnabled = false;
        std::optional<ConeParams> ConeParamsValue;
        std::vector<CAkConversionTable> Curves;
        std::vector<AkRtpc> RTPCs;

        // CAkAttenuation::SetInitialValues
        explicit HierarchyAttenuation(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            if (Ar.Version > 136)
            {
                IsHeightSpreadEnabled = Ar.ReadBool();
            }

            IsConeEnabled = (Ar.Read<uint8_t>() & 1) != 0;
            if (IsConeEnabled)
            {
                ConeParamsValue = ConeParams(Ar);
            }

            // A fixed-size table of curve *slots* whose length grew with the format; the entries are
            // signed indices into the curve array that follows, and C# reads them only to skip them.
            int numCurves;
            if (Ar.Version <= 62)       numCurves = 5;
            else if (Ar.Version <= 72)  numCurves = 4;
            else if (Ar.Version <= 89)  numCurves = 5;
            else if (Ar.Version <= 141) numCurves = 7;
            else if (Ar.Version <= 154) numCurves = 19;
            else                        numCurves = 24;

            Ar.ReadArray<int8_t>(numCurves); // curvesToUse

            int numCurvesFinal;
            if (Ar.Version <= 36)
                numCurvesFinal = static_cast<int>(Ar.Read<uint32_t>()); // Use uint for legacy versions and cast to int
            else
                numCurvesFinal = Ar.Read<uint8_t>(); // Use byte for modern versions

            Curves = Ar.ReadArrayWith(numCurvesFinal, [&Ar] { return CAkConversionTable(Ar); });
            RTPCs = AkRtpc::ReadArray(Ar);
        }
    };
}
