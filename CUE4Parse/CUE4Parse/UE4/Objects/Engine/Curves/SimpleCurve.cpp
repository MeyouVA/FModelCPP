// Ported from CUE4Parse/UE4/Objects/Engine/Curves/SimpleCurve.cs.
#include "SimpleCurve.h"

#include <cmath>
#include <limits>

#include "../../../../Utils/MathUtils.h"
#include "../../Core/Math/UnrealMathUtility.h"
#include "../../../Assets/Objects/FStructFallback.h"
#include "../../../Assets/Objects/FPropertyTag.h"
#include "../../../Assets/Objects/Properties/ArrayProperty.h"
#include "../../../Assets/Objects/Properties/StructProperty.h"

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Objects::Properties::ArrayProperty;
    using CUE4Parse::UE4::Assets::Objects::Properties::StructProperty;
    namespace UM = CUE4Parse::UE4::Objects::Core::Math::UnrealMath;

    namespace
    {
        ERichCurveInterpMode ParseInterpMode(const std::string& member, ERichCurveInterpMode def)
        {
            if (member == "RCIM_Linear") return ERichCurveInterpMode::RCIM_Linear;
            if (member == "RCIM_Constant") return ERichCurveInterpMode::RCIM_Constant;
            if (member == "RCIM_Cubic") return ERichCurveInterpMode::RCIM_Cubic;
            if (member == "RCIM_None") return ERichCurveInterpMode::RCIM_None;
            return def;
        }
    }

    FSimpleCurve::FSimpleCurve(const FStructFallback& data) : FRealCurve(data)
    {
        std::string member;
        bool hasByte;
        uint8_t byteVal;
        if (TryGetEnumField(data, "InterpMode", member, hasByte, byteVal))
            InterpMode = hasByte ? static_cast<ERichCurveInterpMode>(byteVal)
                                 : ParseInterpMode(member, ERichCurveInterpMode::RCIM_Linear);

        // Keys: an ArrayProperty of StructProperty(FSimpleCurveKey); each element is an FStructFallback (the
        // named-struct table is deferred) holding tagged Time + Value floats.
        for (const auto& tag : data.Properties)
        {
            if (tag.Name.Text() != "Keys")
                continue;
            if (const auto* ap = dynamic_cast<const ArrayProperty*>(tag.Tag.get()))
            {
                for (const auto& elem : ap->Value.Properties)
                {
                    const auto* sp = dynamic_cast<const StructProperty*>(elem.get());
                    if (sp == nullptr || sp->Value.Struct == nullptr)
                        continue;
                    const FStructFallback& kd = *sp->Value.Struct;
                    FSimpleCurveKey key;
                    key.Time = GetFloat(kd, "Time", 0.0f);
                    key.Value = GetFloat(kd, "Value", 0.0f);
                    Keys.push_back(key);
                }
            }
            break;
        }
    }

    void FSimpleCurve::RemapTimeValue(float& inTime, float& cycleValueOffset) const
    {
        const int numKeys = static_cast<int>(Keys.size());
        if (numKeys < 2) return;

        if (inTime <= Keys[0].Time)
        {
            if (PreInfinityExtrap != ERichCurveExtrapolation::RCCE_Linear &&
                PreInfinityExtrap != ERichCurveExtrapolation::RCCE_Constant)
            {
                const float minTime = Keys[0].Time;
                const float maxTime = Keys[numKeys - 1].Time;
                int cycleCount = 0;
                CycleTime(minTime, maxTime, inTime, cycleCount);

                if (PreInfinityExtrap == ERichCurveExtrapolation::RCCE_CycleWithOffset)
                {
                    const float dv = Keys[0].Value - Keys[numKeys - 1].Value;
                    cycleValueOffset = dv * cycleCount;
                }
                else if (PreInfinityExtrap == ERichCurveExtrapolation::RCCE_Oscillate)
                {
                    if (cycleCount % 2 == 1)
                        inTime = minTime + (maxTime - inTime);
                }
            }
        }
        else if (inTime >= Keys[numKeys - 1].Time)
        {
            if (PostInfinityExtrap != ERichCurveExtrapolation::RCCE_Linear &&
                PostInfinityExtrap != ERichCurveExtrapolation::RCCE_Constant)
            {
                const float minTime = Keys[0].Time;
                const float maxTime = Keys[numKeys - 1].Time;
                int cycleCount = 0;
                CycleTime(minTime, maxTime, inTime, cycleCount);

                if (PostInfinityExtrap == ERichCurveExtrapolation::RCCE_CycleWithOffset)
                {
                    const float dv = Keys[numKeys - 1].Value - Keys[0].Value;
                    cycleValueOffset = dv * cycleCount;
                }
                else if (PostInfinityExtrap == ERichCurveExtrapolation::RCCE_Oscillate)
                {
                    if (cycleCount % 2 == 1)
                        inTime = minTime + (maxTime - inTime);
                }
            }
        }
    }

    float FSimpleCurve::Eval(float inTime, float inDefaultValue) const
    {
        float cycleValueOffset = 0.0f;
        RemapTimeValue(inTime, cycleValueOffset);

        const int numKeys = static_cast<int>(Keys.size());

        // If the default value hasn't been initialized, use the incoming default value.
        float interpVal = DefaultValue == std::numeric_limits<float>::max() ? inDefaultValue : DefaultValue;

        if (numKeys == 0)
        {
            // interpVal stays as the default
        }
        else if (numKeys < 2 || inTime <= Keys[0].Time)
        {
            if (PreInfinityExtrap == ERichCurveExtrapolation::RCCE_Linear && numKeys > 1)
            {
                const float dt = Keys[1].Time - Keys[0].Time;
                if (std::fabs(dt) <= UM::SmallNumber)
                {
                    interpVal = Keys[0].Value;
                }
                else
                {
                    const float dv = Keys[1].Value - Keys[0].Value;
                    const float slope = dv / dt;
                    interpVal = slope * (inTime - Keys[0].Time) + Keys[0].Value;
                }
            }
            else
            {
                interpVal = Keys[0].Value;
            }
        }
        else if (inTime < Keys[numKeys - 1].Time)
        {
            // lower bound to get the second interpolation node
            int first = 1;
            const int last = numKeys - 1;
            int count = last - first;
            while (count > 0)
            {
                const int step = count / 2;
                const int middle = first + step;
                if (inTime >= Keys[middle].Time)
                {
                    first = middle + 1;
                    count -= step + 1;
                }
                else
                {
                    count = step;
                }
            }
            interpVal = EvalForTwoKeys(Keys[first - 1], Keys[first], inTime);
        }
        else
        {
            if (PostInfinityExtrap == ERichCurveExtrapolation::RCCE_Linear)
            {
                const float dt = Keys[numKeys - 2].Time - Keys[numKeys - 1].Time;
                if (std::fabs(dt) <= UM::SmallNumber)
                {
                    interpVal = Keys[numKeys - 1].Value;
                }
                else
                {
                    const float dv = Keys[numKeys - 2].Value - Keys[numKeys - 1].Value;
                    const float slope = dv / dt;
                    interpVal = slope * (inTime - Keys[numKeys - 1].Time) + Keys[numKeys - 1].Value;
                }
            }
            else
            {
                interpVal = Keys[numKeys - 1].Value;
            }
        }

        return interpVal + cycleValueOffset;
    }

    float FSimpleCurve::EvalForTwoKeys(const FSimpleCurveKey& key1, const FSimpleCurveKey& key2, float inTime) const
    {
        const float diff = key2.Time - key1.Time;
        if (diff > 0.0f && InterpMode != ERichCurveInterpMode::RCIM_Constant)
        {
            const float alpha = (inTime - key1.Time) / diff;
            return CUE4Parse::Utils::Lerp(key1.Value, key2.Value, alpha);
        }
        return key1.Value;
    }
}
