// Ported from CUE4Parse/UE4/Objects/Engine/Curves/SimpleCurve.cs
// A simple float curve: a single interp mode + an array of (Time, Value) keys.
#pragma once

#include <vector>

#include "RealCurve.h"

namespace CUE4Parse::UE4::Assets::Objects { class FStructFallback; }

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    struct FSimpleCurveKey
    {
        float Time = 0.0f;
        float Value = 0.0f;
    };

    class FSimpleCurve : public FRealCurve
    {
    public:
        ERichCurveInterpMode InterpMode = ERichCurveInterpMode::RCIM_Linear;
        std::vector<FSimpleCurveKey> Keys;

        FSimpleCurve() = default;
        explicit FSimpleCurve(const CUE4Parse::UE4::Assets::Objects::FStructFallback& data);

        void RemapTimeValue(float& inTime, float& cycleValueOffset) const override;
        float Eval(float inTime, float inDefaultValue = 0.0f) const override;

    private:
        float EvalForTwoKeys(const FSimpleCurveKey& key1, const FSimpleCurveKey& key2, float inTime) const;
    };
}
