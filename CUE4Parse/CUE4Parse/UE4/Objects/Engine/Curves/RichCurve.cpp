// Ported from CUE4Parse/UE4/Objects/Engine/Curves/RichCurve.cs.
#include "RichCurve.h"

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
    using CUE4Parse::Utils::CubicCurve2D;
    namespace UM = CUE4Parse::UE4::Objects::Core::Math::UnrealMath;

    namespace
    {
        ERichCurveInterpMode ParseInterpMode(const std::string& m)
        {
            if (m == "RCIM_Constant") return ERichCurveInterpMode::RCIM_Constant;
            if (m == "RCIM_Cubic") return ERichCurveInterpMode::RCIM_Cubic;
            if (m == "RCIM_None") return ERichCurveInterpMode::RCIM_None;
            return ERichCurveInterpMode::RCIM_Linear;
        }
        ERichCurveTangentMode ParseTangentMode(const std::string& m)
        {
            if (m == "RCTM_User") return ERichCurveTangentMode::RCTM_User;
            if (m == "RCTM_Break") return ERichCurveTangentMode::RCTM_Break;
            if (m == "RCTM_None") return ERichCurveTangentMode::RCTM_None;
            return ERichCurveTangentMode::RCTM_Auto;
        }
        ERichCurveTangentWeightMode ParseTangentWeightMode(const std::string& m)
        {
            if (m == "RCTWM_WeightedArrive") return ERichCurveTangentWeightMode::RCTWM_WeightedArrive;
            if (m == "RCTWM_WeightedLeave") return ERichCurveTangentWeightMode::RCTWM_WeightedLeave;
            if (m == "RCTWM_WeightedBoth") return ERichCurveTangentWeightMode::RCTWM_WeightedBoth;
            return ERichCurveTangentWeightMode::RCTWM_WeightedNone;
        }
    }

    FRichCurveKey FRichCurve::ReadKey(const FStructFallback& kd)
    {
        FRichCurveKey key;
        key.Time = GetFloat(kd, "Time", 0.0f);
        key.Value = GetFloat(kd, "Value", 0.0f);
        key.ArriveTangent = GetFloat(kd, "ArriveTangent", 0.0f);
        key.ArriveTangentWeight = GetFloat(kd, "ArriveTangentWeight", 0.0f);
        key.LeaveTangent = GetFloat(kd, "LeaveTangent", 0.0f);
        key.LeaveTangentWeight = GetFloat(kd, "LeaveTangentWeight", 0.0f);

        std::string member;
        bool hasByte;
        uint8_t byteVal;
        if (TryGetEnumField(kd, "InterpMode", member, hasByte, byteVal))
            key.InterpMode = hasByte ? static_cast<ERichCurveInterpMode>(byteVal) : ParseInterpMode(member);
        if (TryGetEnumField(kd, "TangentMode", member, hasByte, byteVal))
            key.TangentMode = hasByte ? static_cast<ERichCurveTangentMode>(byteVal) : ParseTangentMode(member);
        if (TryGetEnumField(kd, "TangentWeightMode", member, hasByte, byteVal))
            key.TangentWeightMode = hasByte ? static_cast<ERichCurveTangentWeightMode>(byteVal)
                                            : ParseTangentWeightMode(member);
        return key;
    }

    FRichCurve::FRichCurve(const FStructFallback& data) : FRealCurve(data)
    {
        for (const auto& tag : data.Properties)
        {
            if (tag.Name.Text() != "Keys")
                continue;
            if (const auto* ap = dynamic_cast<const ArrayProperty*>(tag.Tag.get()))
            {
                for (const auto& elem : ap->Value.Properties)
                {
                    const auto* sp = dynamic_cast<const StructProperty*>(elem.get());
                    if (sp == nullptr)
                        continue;
                    // "RichCurveKey" is in the named-struct table, so a real asset yields the struct
                    // directly (C#'s data.GetOrDefault<FRichCurveKey[]>). Data that was written as tagged
                    // properties still resolves through the fallback bag.
                    if (const auto* key = sp->Value.Get<FRichCurveKey>())
                        Keys.push_back(*key);
                    else if (const auto* fallback = sp->Value.AsFallback())
                        Keys.push_back(ReadKey(*fallback));
                }
            }
            break;
        }
    }

    void FRichCurve::RemapTimeValue(float& inTime, float& cycleValueOffset) const
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

    float FRichCurve::Eval(float inTime, float inDefaultValue) const
    {
        float cycleValueOffset = 0.0f;
        RemapTimeValue(inTime, cycleValueOffset);

        const int numKeys = static_cast<int>(Keys.size());
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

    float FRichCurve::EvalForTwoKeys(const FRichCurveKey& key1, const FRichCurveKey& key2, float inTime) const
    {
        const float diff = key2.Time - key1.Time;

        if (diff > 0.0f && key1.InterpMode != ERichCurveInterpMode::RCIM_Constant)
        {
            const float alpha = (inTime - key1.Time) / diff;
            const float p0 = key1.Value;
            const float p3 = key2.Value;

            if (key1.InterpMode == ERichCurveInterpMode::RCIM_Linear)
                return CUE4Parse::Utils::Lerp(p0, p3, alpha);

            if (IsItNotWeighted(key1, key2))
            {
                constexpr float oneThird = 1.0f / 3.0f;
                const float p1 = p0 + key1.LeaveTangent * diff * oneThird;
                const float p2 = p3 - key2.ArriveTangent * diff * oneThird;
                return BezierInterp(p0, p1, p2, p3, alpha);
            }

            // it's weighted
            return WeightedEvalForTwoKeys(key1, key2, inTime);
        }

        return key1.Value;
    }

    float FRichCurve::WeightedEvalForTwoKeys(const FRichCurveKey& key1, const FRichCurveKey& key2, float inTime) const
    {
        const float diff = key2.Time - key1.Time;
        const float alpha = (inTime - key1.Time) / diff;
        const float p0 = key1.Value;
        const float p3 = key2.Value;
        const double oneThird = 1.0 / 3.0;
        const float time1 = key1.Time;
        const float time2 = key2.Time;
        const float x = time2 - time1;
        double angle = std::atan(static_cast<double>(key1.LeaveTangent));
        double cosAngle = std::cos(angle);
        double sinAngle = std::sin(angle);

        double leaveWeight = key1.LeaveTangentWeight;
        if (key1.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedNone ||
            key1.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedArrive)
        {
            const float leaveTangentNormalized = key1.LeaveTangent;
            const float y = leaveTangentNormalized * x;
            leaveWeight = std::sqrt(static_cast<double>(x) * x + static_cast<double>(y) * y) * oneThird;
        }

        const double key1TanX = cosAngle * leaveWeight + time1;
        const double key1TanY = sinAngle * leaveWeight + key1.Value;
        angle = std::atan(static_cast<double>(key2.ArriveTangent));
        cosAngle = std::cos(angle);
        sinAngle = std::cos(angle); // NOTE: preserves the CUE4Parse quirk (Cos, not Sin) for the arrive tangent.

        double arriveWeight = key2.ArriveTangentWeight;
        if (key2.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedNone ||
            key2.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedLeave)
        {
            const float arriveTangentNormalized = key2.ArriveTangent;
            const float y = arriveTangentNormalized * x;
            arriveWeight = std::sqrt(static_cast<double>(x) * x + static_cast<double>(y) * y) * oneThird;
        }

        const double key2TanX = -cosAngle * arriveWeight + time2;
        const double key2TanY = -sinAngle * arriveWeight + key2.Value;

        // Normalize the time range
        const double rangeX = time2 - time1;
        const double dx1 = key1TanX - time1;
        const double dx2 = key2TanX - time1;
        const double normalizedX1 = dx1 / rangeX;
        const double normalizedX2 = dx2 / rangeX;

        double results[3] = {0.0, 0.0, 0.0};
        double coeff[4];
        BezierToPower(0.0, normalizedX1, normalizedX2, 1.0, coeff);
        coeff[0] -= alpha;

        const int numResults = CubicCurve2D::SolveCubic(coeff, results);
        float newInterp;

        if (numResults == 1)
        {
            newInterp = static_cast<float>(results[0]);
        }
        else
        {
            newInterp = std::numeric_limits<float>::lowest();
            for (int i = 0; i < numResults; ++i)
            {
                const double result = results[i];
                if (result >= 0.0 && result <= 1.0)
                {
                    if (newInterp < 0.0f || result > newInterp)
                        newInterp = static_cast<float>(result);
                }
            }
            if (newInterp == std::numeric_limits<float>::lowest())
                newInterp = 0.0f;
        }

        return BezierInterp(p0, static_cast<float>(key1TanY), static_cast<float>(key2TanY), p3, newInterp);
    }

    void FRichCurve::BezierToPower(double a1, double b1, double c1, double d1, double output[4])
    {
        const double a = b1 - a1;
        const double b = c1 - b1;
        const double c = d1 - c1;
        const double d = b - a;
        output[3] = c - b - d;
        output[2] = 3.0 * d;
        output[1] = 3.0 * a;
        output[0] = a1;
    }

    float FRichCurve::BezierInterp(float p0, float p1, float p2, float p3, float alpha)
    {
        using CUE4Parse::Utils::Lerp;
        const float p01 = Lerp(p0, p1, alpha);
        const float p12 = Lerp(p1, p2, alpha);
        const float p23 = Lerp(p2, p3, alpha);
        const float p012 = Lerp(p01, p12, alpha);
        const float p123 = Lerp(p12, p23, alpha);
        return Lerp(p012, p123, alpha);
    }

    bool FRichCurve::IsItNotWeighted(const FRichCurveKey& key1, const FRichCurveKey& key2)
    {
        const bool k1 = key1.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedNone ||
                        key1.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedArrive;
        const bool k2 = key2.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedNone ||
                        key2.TangentWeightMode == ERichCurveTangentWeightMode::RCTWM_WeightedLeave;
        return k1 && k2;
    }
}
