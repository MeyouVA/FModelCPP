// Ported from CUE4Parse/UE4/Objects/Engine/Curves/RealCurve.cs.
#include "RealCurve.h"

#include <cmath>
#include <cstddef>

#include "../../../Assets/Objects/FStructFallback.h"
#include "../../../Assets/Objects/FPropertyTag.h"
#include "../../../Assets/Objects/Properties/FloatProperty.h"
#include "../../../Assets/Objects/Properties/EnumProperty.h"
#include "../../../Assets/Objects/Properties/ByteProperty.h"

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Objects::Properties::FloatProperty;
    using CUE4Parse::UE4::Assets::Objects::Properties::EnumProperty;
    using CUE4Parse::UE4::Assets::Objects::Properties::ByteProperty;

    namespace
    {
        // Maps an ERichCurveExtrapolation member name (the part after "::") to its enum value.
        ERichCurveExtrapolation ParseExtrap(const std::string& member, ERichCurveExtrapolation def)
        {
            if (member == "RCCE_Cycle") return ERichCurveExtrapolation::RCCE_Cycle;
            if (member == "RCCE_CycleWithOffset") return ERichCurveExtrapolation::RCCE_CycleWithOffset;
            if (member == "RCCE_Oscillate") return ERichCurveExtrapolation::RCCE_Oscillate;
            if (member == "RCCE_Linear") return ERichCurveExtrapolation::RCCE_Linear;
            if (member == "RCCE_Constant") return ERichCurveExtrapolation::RCCE_Constant;
            if (member == "RCCE_None") return ERichCurveExtrapolation::RCCE_None;
            return def;
        }
    }

    FRealCurve::FRealCurve(const FStructFallback& data)
    {
        DefaultValue = GetFloat(data, "DefaultValue", 3.402823466e+38f);

        std::string member;
        bool hasByte;
        uint8_t byteVal;
        if (TryGetEnumField(data, "PreInfinityExtrap", member, hasByte, byteVal))
            PreInfinityExtrap = hasByte ? static_cast<ERichCurveExtrapolation>(byteVal)
                                        : ParseExtrap(member, ERichCurveExtrapolation::RCCE_Constant);
        if (TryGetEnumField(data, "PostInfinityExtrap", member, hasByte, byteVal))
            PostInfinityExtrap = hasByte ? static_cast<ERichCurveExtrapolation>(byteVal)
                                         : ParseExtrap(member, ERichCurveExtrapolation::RCCE_Constant);
    }

    void FRealCurve::CycleTime(float minTime, float maxTime, float& inTime, int& cycleCount)
    {
        const float initTime = inTime;
        const float duration = maxTime - minTime;

        if (inTime > maxTime)
        {
            cycleCount = static_cast<int>((maxTime - inTime) / duration);
            inTime += duration * cycleCount;
        }
        else if (inTime < minTime)
        {
            cycleCount = static_cast<int>((inTime - minTime) / duration);
            inTime -= duration * cycleCount;
        }

        if (inTime == maxTime && initTime < minTime)
            inTime = minTime;
        if (inTime == minTime && initTime > maxTime)
            inTime = maxTime;

        cycleCount = std::abs(cycleCount);
    }

    float FRealCurve::GetFloat(const FStructFallback& data, const char* name, float def)
    {
        for (const auto& tag : data.Properties)
        {
            if (tag.Name.Text() != name)
                continue;
            if (const auto* fp = dynamic_cast<const FloatProperty*>(tag.Tag.get()))
                return fp->Value;
            break;
        }
        return def;
    }

    bool FRealCurve::TryGetEnumField(const FStructFallback& data, const char* name,
                                     std::string& outMember, bool& outHasByte, uint8_t& outByte)
    {
        for (const auto& tag : data.Properties)
        {
            if (tag.Name.Text() != name)
                continue;
            if (const auto* ep = dynamic_cast<const EnumProperty*>(tag.Tag.get()))
            {
                const std::string full = ep->Value.Text();
                const std::size_t sep = full.rfind("::");
                outMember = sep == std::string::npos ? full : full.substr(sep + 2);
                outHasByte = false;
                return true;
            }
            if (const auto* bp = dynamic_cast<const ByteProperty*>(tag.Tag.get()))
            {
                outHasByte = true;
                outByte = bp->Value;
                return true;
            }
            break;
        }
        return false;
    }
}
