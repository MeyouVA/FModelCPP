// Ported from CUE4Parse/UE4/Objects/Engine/Curves/RealCurve.cs
// FRealCurve is the abstract base of FSimpleCurve / FRichCurve: a float curve with a default value and pre/post
// infinity extrapolation. Concrete curves add their key arrays + Eval.
//
// Deliberate differences from C#:
//   * C# reads fields from an FStructFallback via reflection (data.GetOrDefault(nameof(X), ...)). This port has
//     no reflection accessors, so the shared static helpers GetFloat / TryGetEnumField scan the fallback's
//     tagged Properties list and dynamic_cast (mirrors the UDataTable / UUserDefinedStruct Status pattern).
//   * The archive binary-read ctors (FRichCurve(FMutableArchive), FRichCurveKey(FMutableArchive)) and
//     FCompressedRichCurve + its AnimCurveCompressionCodec ConverterMap (unsafe pointer key adapters) are
//     deferred with the animation-compression arc; curve tables serialize their curves as tagged FStructFallback
//     data, which is what is ported here. TODO.
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::UE4::Assets::Objects { class FStructFallback; }

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    /** Method of interpolation between this key and the next. */
    enum class ERichCurveInterpMode : uint8_t
    {
        RCIM_Linear,
        RCIM_Constant,
        RCIM_Cubic,
        RCIM_None
    };

    /** Enumerates extrapolation options. */
    enum class ERichCurveExtrapolation : uint8_t
    {
        RCCE_Cycle,
        RCCE_CycleWithOffset,
        RCCE_Oscillate,
        RCCE_Linear,
        RCCE_Constant,
        RCCE_None
    };

    /** A rich, editable float curve (abstract). */
    class FRealCurve
    {
    public:
        float DefaultValue = 3.402823466e+38f; // MAX_flt
        ERichCurveExtrapolation PreInfinityExtrap = ERichCurveExtrapolation::RCCE_Constant;
        ERichCurveExtrapolation PostInfinityExtrap = ERichCurveExtrapolation::RCCE_Constant;

        FRealCurve() = default;
        explicit FRealCurve(const CUE4Parse::UE4::Assets::Objects::FStructFallback& data);
        virtual ~FRealCurve() = default;

        virtual void RemapTimeValue(float& inTime, float& cycleValueOffset) const = 0;
        virtual float Eval(float inTime, float inDefaultValue = 0.0f) const = 0;

    protected:
        static void CycleTime(float minTime, float maxTime, float& inTime, int& cycleCount);

        // FStructFallback field extraction (replaces C#'s reflection GetOrDefault). Scans `data`'s tagged
        // Properties for a tag named `name`.
        //   GetFloat        -> a FloatProperty's value, else `def`.
        //   TryGetEnumField -> a tag stored as an EnumProperty (member name after "::" in outMember) or a
        //                      ByteProperty (outHasByte=true, outByte). Returns false if the tag is absent.
        static float GetFloat(const CUE4Parse::UE4::Assets::Objects::FStructFallback& data,
                              const char* name, float def);
        static bool TryGetEnumField(const CUE4Parse::UE4::Assets::Objects::FStructFallback& data,
                                    const char* name, std::string& outMember, bool& outHasByte, uint8_t& outByte);
    };
}
