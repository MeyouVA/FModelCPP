// Ported from CUE4Parse/UE4/Assets/Objects/FScriptStruct.cs — the named-struct switch (ported entries only,
// see the deferral list in FScriptStruct.h) plus the FStructFallback default arm and ToString.
#include "FScriptStruct.h"

#include "../Readers/FAssetArchive.h"
#include "Properties/FPropertyTagType.h"

#include "../../Objects/Core/Math/FBox.h"
#include "../../Objects/Core/Math/FBox2D.h"
#include "../../Objects/Core/Math/FCapsuleShape.h"
#include "../../Objects/Core/Math/FColor.h"
#include "../../Objects/Core/Math/FIntPoint.h"
#include "../../Objects/Core/Math/FIntVector.h"
#include "../../Objects/Core/Math/FLinearColor.h"
#include "../../Objects/Core/Math/FPlane.h"
#include "../../Objects/Core/Math/FQuat.h"
#include "../../Objects/Core/Math/FRotator.h"
#include "../../Objects/Core/Math/FSphere.h"
#include "../../Objects/Core/Math/FTransform.h"
#include "../../Objects/Core/Math/FTwoVectors.h"
#include "../../Objects/Core/Math/FVector.h"
#include "../../Objects/Core/Math/FVector2D.h"
#include "../../Objects/Core/Math/FVector4.h"
#include "../../Objects/Core/Math/Matrix.h"
#include "../../Objects/Core/Math/TBox3.h"
#include "../../Objects/Core/Math/TIntVector.h"
#include "../../Objects/Core/Misc/FDateTime.h"
#include "../../Objects/Core/Misc/FFrameNumber.h"
#include "../../Objects/Core/Misc/FGuid.h"
#include "../../Objects/Engine/Curves/RichCurve.h"
#include "../../Objects/Engine/Curves/SimpleCurve.h"
#include "../../Objects/UObject/FSoftObjectPath.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using namespace CUE4Parse::UE4::Objects::Core::Math;
    using CUE4Parse::UE4::Objects::Core::Misc::FDateTime;
    using CUE4Parse::UE4::Objects::Core::Misc::FFrameNumber;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::Engine::Curves::FRichCurveKey;
    using CUE4Parse::UE4::Objects::Engine::Curves::FSimpleCurveKey;
    using CUE4Parse::UE4::Objects::UObject::FSoftObjectPath;
    using Properties::ReadType;

    FScriptStruct::FScriptStruct(Readers::FAssetArchive& Ar, const std::optional<std::string>& structName,
                                 const CUE4Parse::UE4::Objects::UObject::UStruct* struc, ReadType type)
    {
        const std::string name = structName.has_value() ? *structName : std::string();
        const bool zero = type == ReadType::ZERO;

        // C#'s switch arms, in source order. `zero` is C#'s `type == ReadType.ZERO ? new T() : <read>`.
        if (name == "Box") StructType = zero ? Make<FBox>() : Make<FBox>(Ar);
        else if (name == "Box2D") StructType = zero ? Make<FBox2D>() : Make<FBox2D>(Ar);
        else if (name == "Box2f") StructType = zero ? Make<TBox2<float>>() : Make<TBox2<float>>(Ar);
        else if (name == "Box3f") StructType = zero ? Make<TBox3<float>>() : Make<TBox3<float>>(Ar);
        else if (name == "CapsuleShape") StructType = zero ? Make<FCapsuleShape>() : Make<FCapsuleShape>(Ar);
        else if (name == "Color") StructType = zero ? Make<FColor>() : Make<FColor>(Ar.Read<FColor>());
        else if (name == "DateTime" || name == "Timespan")
            StructType = zero ? Make<FDateTime>() : Make<FDateTime>(Ar.Read<FDateTime>());
        else if (name == "FrameNumber") StructType = zero ? Make<FFrameNumber>() : Make<FFrameNumber>(Ar.Read<FFrameNumber>());
        else if (name == "Guid") StructType = zero ? Make<FGuid>() : Make<FGuid>(Ar.Read<FGuid>());
        else if (name == "RichCurveKey") StructType = zero ? Make<FRichCurveKey>() : Make<FRichCurveKey>(Ar.Read<FRichCurveKey>());
        else if (name == "SimpleCurveKey") StructType = zero ? Make<FSimpleCurveKey>() : Make<FSimpleCurveKey>(Ar.Read<FSimpleCurveKey>());
        else if (name == "IntPoint" || name == "Int32Point")
            StructType = zero ? Make<FIntPoint>() : Make<FIntPoint>(Ar.Read<FIntPoint>());
        else if (name == "IntVector2" || name == "Int32Vector2")
            StructType = zero ? Make<TIntVector2<int32_t>>() : Make<TIntVector2<int32_t>>(Ar.Read<TIntVector2<int32_t>>());
        else if (name == "UintVector2" || name == "Uint32Point")
            StructType = zero ? Make<TIntVector2<uint32_t>>() : Make<TIntVector2<uint32_t>>(Ar.Read<TIntVector2<uint32_t>>());
        else if (name == "IntVector") StructType = zero ? Make<FIntVector>() : Make<FIntVector>(Ar.Read<FIntVector>());
        else if (name == "UintVector")
            StructType = zero ? Make<TIntVector3<uint32_t>>() : Make<TIntVector3<uint32_t>>(Ar.Read<TIntVector3<uint32_t>>());
        else if (name == "IntVector4")
            StructType = zero ? Make<TIntVector4<int32_t>>() : Make<TIntVector4<int32_t>>(Ar.Read<TIntVector4<int32_t>>());
        else if (name == "UintVector4")
            StructType = zero ? Make<TIntVector4<uint32_t>>() : Make<TIntVector4<uint32_t>>(Ar.Read<TIntVector4<uint32_t>>());
        else if (name == "Int64Vector2" || name == "Int64Point")
            StructType = zero ? Make<TIntVector2<int64_t>>() : Make<TIntVector2<int64_t>>(Ar.Read<TIntVector2<int64_t>>());
        else if (name == "UInt64Vector2" || name == "UInt64Point")
            StructType = zero ? Make<TIntVector2<uint64_t>>() : Make<TIntVector2<uint64_t>>(Ar.Read<TIntVector2<uint64_t>>());
        else if (name == "Int64Vector")
            StructType = zero ? Make<TIntVector3<int64_t>>() : Make<TIntVector3<int64_t>>(Ar.Read<TIntVector3<int64_t>>());
        else if (name == "UInt64Vector")
            StructType = zero ? Make<TIntVector3<uint64_t>>() : Make<TIntVector3<uint64_t>>(Ar.Read<TIntVector3<uint64_t>>());
        else if (name == "Int64Vector4")
            StructType = zero ? Make<TIntVector4<int64_t>>() : Make<TIntVector4<int64_t>>(Ar.Read<TIntVector4<int64_t>>());
        else if (name == "UInt64Vector4")
            StructType = zero ? Make<TIntVector4<uint64_t>>() : Make<TIntVector4<uint64_t>>(Ar.Read<TIntVector4<uint64_t>>());
        else if (name == "LinearColor") StructType = zero ? Make<FLinearColor>() : Make<FLinearColor>(Ar.Read<FLinearColor>());
        else if (name == "Plane") StructType = zero ? Make<FPlane>() : Make<FPlane>(Ar);
        else if (name == "Plane4f")
        {
            if (zero) StructType = Make<FPlane>();
            else
            {
                // Sequenced through locals: C# reads the vector then W, and C++ leaves argument evaluation
                // order unspecified.
                const auto v = Ar.Read<TIntVector3<float>>();
                const auto w = Ar.Read<float>();
                StructType = Make<FPlane>(v.X, v.Y, v.Z, w);
            }
        }
        else if (name == "Plane4d")
        {
            if (zero) StructType = Make<FPlane>();
            else
            {
                const auto v = Ar.Read<TIntVector3<double>>();
                const auto w = Ar.Read<double>();
                StructType = Make<FPlane>(static_cast<float>(v.X), static_cast<float>(v.Y),
                                          static_cast<float>(v.Z), static_cast<float>(w));
            }
        }
        else if (name == "Quat") StructType = zero ? Make<FQuat>() : Make<FQuat>(Ar);
        else if (name == "Quat4f")
        {
            if (zero) StructType = Make<FQuat>();
            else { const auto v = Ar.Read<TIntVector4<float>>(); StructType = Make<FQuat>(v.X, v.Y, v.Z, v.W); }
        }
        else if (name == "Quat4d")
        {
            if (zero) StructType = Make<FQuat>();
            else
            {
                const auto v = Ar.Read<TIntVector4<double>>();
                StructType = Make<FQuat>(static_cast<float>(v.X), static_cast<float>(v.Y),
                                         static_cast<float>(v.Z), static_cast<float>(v.W));
            }
        }
        else if (name == "Rotator") StructType = zero ? Make<FRotator>() : Make<FRotator>(Ar);
        else if (name == "Rotator3f")
        {
            if (zero) StructType = Make<FRotator>();
            else
            {
                const auto pitch = Ar.Read<float>();
                const auto yaw = Ar.Read<float>();
                const auto roll = Ar.Read<float>();
                StructType = Make<FRotator>(pitch, yaw, roll);
            }
        }
        else if (name == "Rotator3d")
        {
            if (zero) StructType = Make<FRotator>();
            else
            {
                const auto pitch = Ar.Read<double>();
                const auto yaw = Ar.Read<double>();
                const auto roll = Ar.Read<double>();
                StructType = Make<FRotator>(static_cast<float>(pitch), static_cast<float>(yaw), static_cast<float>(roll));
            }
        }
        else if (name == "Sphere") StructType = zero ? Make<FSphere>() : Make<FSphere>(Ar);
        else if (name == "Sphere3f")
        {
            if (zero) StructType = Make<FSphere>();
            else
            {
                const auto center = Ar.Read<TIntVector3<float>>();
                const auto w = Ar.Read<float>();
                StructType = Make<FSphere>(center, w);
            }
        }
        else if (name == "Sphere3d")
        {
            if (zero) StructType = Make<FSphere>();
            else
            {
                const auto center = Ar.Read<TIntVector3<double>>();
                const auto w = Ar.Read<double>();
                StructType = Make<FSphere>(center, w);
            }
        }
        else if (name == "StringClassReference" || name == "SoftClassPath" ||
                 name == "StringAssetReference" || name == "SoftObjectPath")
            StructType = zero ? Make<FSoftObjectPath>() : Make<FSoftObjectPath>(Ar);
        else if (name == "Transform3f") StructType = zero ? Make<FTransform>() : Make<FTransform>(Ar.Read<FTransform>());
        else if (name == "TwoVectors") StructType = zero ? Make<FTwoVectors>() : Make<FTwoVectors>(Ar);
        else if (name == "Vector") StructType = zero ? Make<FVector>() : Make<FVector>(Ar);
        else if (name == "Vector2D") StructType = zero ? Make<FVector2D>() : Make<FVector2D>(Ar);
        else if (name == "Vector2f")
            StructType = zero ? Make<TIntVector2<float>>() : Make<TIntVector2<float>>(Ar.Read<TIntVector2<float>>());
        else if (name == "DeprecateSlateVector2D")
            StructType = zero ? Make<FVector2D>() : Make<FVector2D>(Ar.Read<FVector2D>());
        else if (name == "Vector3f")
            StructType = zero ? Make<TIntVector3<float>>() : Make<TIntVector3<float>>(Ar.Read<TIntVector3<float>>());
        else if (name == "Vector3d" || name == "VectorDouble")
            StructType = zero ? Make<TIntVector3<double>>() : Make<TIntVector3<double>>(Ar.Read<TIntVector3<double>>());
        else if (name == "Vector4") StructType = zero ? Make<FVector4>() : Make<FVector4>(Ar);
        else if (name == "Vector4f")
            StructType = zero ? Make<TIntVector4<float>>() : Make<TIntVector4<float>>(Ar.Read<TIntVector4<float>>());
        else if (name == "Vector4d")
            StructType = zero ? Make<TIntVector4<double>>() : Make<TIntVector4<double>>(Ar.Read<TIntVector4<double>>());
        // UE5 writes these as tagged property bags rather than a bare vector (Versions option).
        // C# names the fallback "Vector_NetQuantize" for all four variants; kept verbatim.
        else if (name == "Vector_NetQuantize" || name == "Vector_NetQuantize10" ||
                 name == "Vector_NetQuantize100" || name == "Vector_NetQuantizeNormal")
        {
            if (zero) StructType = Make<FVector>();
            else if (Ar.Versions["Vector_NetQuantize_AsStruct"])
                StructType = Make<FStructFallback>(Ar, std::optional<std::string>("Vector_NetQuantize"));
            else StructType = Make<FVector>(Ar);
        }
        else if (name == "Matrix") StructType = zero ? Make<FMatrix>() : Make<FMatrix>(Ar);
        else if (name == "Matrix44f") StructType = zero ? Make<FMatrix>() : Make<FMatrix>(Ar, false);
        else if (name == "KeyHandleMap") StructType = Make<FStructFallback>();
        // C#'s `default:` arm (its RAW sub-arm needs the unported raw-header ctor, so RAW lands here too).
        else if (zero) StructType = Make<FStructFallback>();
        else if (struc != nullptr) StructType = Make<FStructFallback>(Ar, struc);
        else StructType = Make<FStructFallback>(Ar, structName);
    }

    std::string FScriptStruct::ToString() const
    {
        if (!StructType) return "(null)";
        return StructType->ToString() + " (" + StructType->TypeName() + ")";
    }
}
