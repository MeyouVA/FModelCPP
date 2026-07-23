// Ported from CUE4Parse/UE4/Assets/Objects/FScriptStruct.cs
// The value of a StructProperty: a resolved struct instance. C# picks an IUStruct out of a ~200-entry switch
// on the struct's name (FVector, FGuid, FColor, ...) and falls back to FStructFallback for everything else.
//
// Deliberate differences from C#:
//   * IUStruct is an empty, non-virtual marker here (see IUStruct.h — the value structs must stay trivially
//     copyable so FArchive::Read<T> works on them), so it cannot be the polymorphic slot C#'s
//     `IUStruct StructType` is. The value is instead owned through IUStructHolder, a tiny virtual box, and
//     read back with Get<T>(). Everything else about the field is the same.
//   * Only the named entries whose struct type is already ported are present; the rest still take the
//     C# `default:` arm (FStructFallback). Notably deferred: every FMaterialInput/FExpressionInput entry, the
//     MovieScene/Niagara/PCG/StateTree/cloth/instanced-struct families, FGameplayTagContainer, FSmartName,
//     FPerPlatform*/FPerQualityLevel*, FRawStruct/FFixedSizeStruct, and all of the per-game (GameTypes)
//     arms. TODO with those types.
//   * The Vector_NetQuantize* entries branch on `Ar.Versions["Vector_NetQuantize_AsStruct"]` like C#, so a
//     UE5 game reads them as a tagged property bag and everything older as a bare FVector.
//   * Plane4f/Plane4d/Quat4f/Quat4d/Sphere3d narrow their components to float, because this port's FPlane /
//     FQuat / FSphere hold floats (C#'s hold the same; its doubles arms are the ones that narrow).
//   * The `struc` (UStruct*) parameter IS ported, but the ReadInstancedStruct* helpers (which need
//     FPackageIndex object loading plus FInstancedStruct) are not. TODO.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>

#include "FStructFallback.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }
namespace CUE4Parse::UE4::Objects::UObject { class UStruct; }

namespace CUE4Parse::UE4::Assets::Objects
{
    namespace Properties { enum class ReadType : uint8_t; }

    // Strips a typeid name down to the bare type name: MSVC spells it "struct Ns::Inner::FVector", and
    // template arguments must survive ("TIntVector3<float>"). Only used for ToString.
    inline std::string PrettyTypeName(const std::type_info& info)
    {
        std::string name = info.name();
        const size_t templateStart = name.find('<');
        const size_t lastScope = name.rfind("::", templateStart);
        if (lastScope != std::string::npos)
            return name.substr(lastScope + 2);
        for (const char* prefix : {"struct ", "class "})
        {
            const std::string p(prefix);
            if (name.rfind(p, 0) == 0) return name.substr(p.size());
        }
        return name;
    }

    // The polymorphic slot C#'s `IUStruct StructType` field is (see the header note).
    class IUStructHolder
    {
    public:
        virtual ~IUStructHolder() = default;
        virtual std::string TypeName() const = 0;
        virtual std::string ToString() const = 0;
    };

    template <typename T>
    concept HasToString = requires(const T& value) { { value.ToString() } -> std::convertible_to<std::string>; };

    template <typename T>
    class TUStructHolder final : public IUStructHolder
    {
    public:
        T Value;

        template <typename... Args>
        explicit TUStructHolder(Args&&... args) : Value(std::forward<Args>(args)...) {}

        std::string TypeName() const override { return PrettyTypeName(typeid(T)); }
        std::string ToString() const override
        {
            if constexpr (HasToString<T>) return Value.ToString();
            else return TypeName();
        }
    };

    class FScriptStruct
    {
    public:
        std::unique_ptr<IUStructHolder> StructType;

        FScriptStruct() = default;
        // C#'s FScriptStruct(Ar, structName, struc, type): the named-struct switch.
        FScriptStruct(Readers::FAssetArchive& Ar, const std::optional<std::string>& structName,
                      const CUE4Parse::UE4::Objects::UObject::UStruct* struc, Properties::ReadType type);
        // C#'s FScriptStruct(IUStruct structType).
        explicit FScriptStruct(std::unique_ptr<IUStructHolder> structType) : StructType(std::move(structType)) {}

        // Move-only (owns a unique_ptr to a move-only holder).
        FScriptStruct(FScriptStruct&&) = default;
        FScriptStruct& operator=(FScriptStruct&&) = default;
        FScriptStruct(const FScriptStruct&) = delete;
        FScriptStruct& operator=(const FScriptStruct&) = delete;

        // C#'s `StructType is T t` pattern: the value when it has exactly this type, null otherwise.
        template <typename T>
        const T* Get() const
        {
            const auto* holder = dynamic_cast<const TUStructHolder<T>*>(StructType.get());
            return holder != nullptr ? &holder->Value : nullptr;
        }

        // The common case of Get<T>: the generic property bag every unnamed struct falls back to.
        const FStructFallback* AsFallback() const { return Get<FStructFallback>(); }

        std::string ToString() const;

        // Convenience for the named entries and for tests: box a value of any struct type.
        template <typename T, typename... Args>
        static std::unique_ptr<IUStructHolder> Make(Args&&... args)
        {
            return std::make_unique<TUStructHolder<T>>(std::forward<Args>(args)...);
        }
    };
}
