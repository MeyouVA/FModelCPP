// Ported from CUE4Parse/UE4/Objects/Engine/Curves/RichCurve.cs
// A rich, editable float curve: an array of keys, each with per-key interp mode + tangents (optionally weighted).
//
// Deliberate differences from C#:
//   * FCompressedRichCurve and its AnimCurveCompressionCodec ConverterMap (the unsafe pointer key-time/key-data
//     adapters + the quantized-key decompressors) are NOT ported — that is the animation-compression arc.
//   * The FMutableArchive binary-read ctors are omitted (curve tables serialize as tagged FStructFallback data).
//   Both TODO.
#pragma once

#include <cstdint>
#include <vector>

#include "RealCurve.h"

namespace CUE4Parse::UE4::Assets::Objects { class FStructFallback; }

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    /** If using RCIM_Cubic, describes how the tangents should be controlled in editor. */
    enum class ERichCurveTangentMode : uint8_t
    {
        RCTM_Auto,
        RCTM_User,
        RCTM_Break,
        RCTM_None
    };

    /** Enumerates tangent weight modes. */
    enum class ERichCurveTangentWeightMode : uint8_t
    {
        RCTWM_WeightedNone,
        RCTWM_WeightedArrive,
        RCTWM_WeightedLeave,
        RCTWM_WeightedBoth
    };

    /** Enumerates curve compression options (present for completeness; the compressed path is deferred). */
    enum class ERichCurveCompressionFormat : uint8_t
    {
        RCCF_Empty,
        RCCF_Constant,
        RCCF_Linear,
        RCCF_Cubic,
        RCCF_Mixed,
        RCCF_Weighted
    };

    /** Enumerates key time compression options. */
    enum class ERichCurveKeyTimeCompressionFormat : uint8_t
    {
        RCKTCF_uint16,
        RCKTCF_float32
    };

    /** One key in a rich, editable float curve. */
    struct FRichCurveKey
    {
        ERichCurveInterpMode InterpMode = ERichCurveInterpMode::RCIM_Linear;
        ERichCurveTangentMode TangentMode = ERichCurveTangentMode::RCTM_Auto;
        ERichCurveTangentWeightMode TangentWeightMode = ERichCurveTangentWeightMode::RCTWM_WeightedNone;
        float Time = 0.0f;
        float Value = 0.0f;
        float ArriveTangent = 0.0f;
        float ArriveTangentWeight = 0.0f;
        float LeaveTangent = 0.0f;
        float LeaveTangentWeight = 0.0f;
    };

    class FRichCurve : public FRealCurve
    {
    public:
        std::vector<FRichCurveKey> Keys;

        FRichCurve() = default;
        explicit FRichCurve(const CUE4Parse::UE4::Assets::Objects::FStructFallback& data);

        void RemapTimeValue(float& inTime, float& cycleValueOffset) const override;
        float Eval(float inTime, float inDefaultValue = 0.0f) const override;

    private:
        static FRichCurveKey ReadKey(const CUE4Parse::UE4::Assets::Objects::FStructFallback& keyData);
        float EvalForTwoKeys(const FRichCurveKey& key1, const FRichCurveKey& key2, float inTime) const;
        float WeightedEvalForTwoKeys(const FRichCurveKey& key1, const FRichCurveKey& key2, float inTime) const;
        static void BezierToPower(double a1, double b1, double c1, double d1, double output[4]);
        static float BezierInterp(float p0, float p1, float p2, float p3, float alpha);
        static bool IsItNotWeighted(const FRichCurveKey& key1, const FRichCurveKey& key2);
    };
}
