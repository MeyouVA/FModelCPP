// Ported from CUE4Parse/UE4/Assets/Exports/UObject.cs (the PropertyUtil static class at the bottom).
// "Give me the property called X as a T." Every cooked asset type in the tree is written against this:
// UAkAudioEvent, USoundWave, the MetaSound frontend structs and so on all read their fields out of the
// tagged-property bag rather than off the wire.
//
// Deliberate differences from C#:
//   * C#'s FPropertyTagType.GetValue(Type) is .NET reflection: it takes a runtime Type and coerces the
//     stored value to it. C++ has no such thing, so the coercion is a compile-time overload set --
//     PropertyValue(tag, T&) below -- with one arm per shape C# handles. The arms are the same ones and in
//     the same order of preference; anything C# would return null for returns false here.
//   * Numeric widening IS allowed between integer property types of the same signedness (C# gets this via
//     the `is T` test failing and callers casting; the port folds the cast in so `GetOrDefault<uint>` off a
//     UInt32Property behaves as the call sites expect). A mismatch in signedness still fails.
//   * SearchPropertyInTemplate (C#'s static opt-in that also searches the Template and Class objects) is
//     ported as the same static flag, but the walk needs ResolvedObject::Object() and so is only performed
//     when the holder is a UObject.
//   * The Set<T>/GetByIndex<T>/GetOrDefaultLazy family is not ported: nothing in the reader layer writes
//     properties back, and Lazy<T> is a C#-ism. TODO if the editor layer ever needs them.
#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "../Objects/FPropertyTag.h"
#include "../Objects/FStructFallback.h"
#include "../Objects/Properties/ArrayProperty.h"
#include "../Objects/Properties/BoolProperty.h"
#include "../Objects/Properties/ByteProperty.h"
#include "../Objects/Properties/EnumProperty.h"
#include "../Objects/Properties/NameProperty.h"
#include "../Objects/Properties/ObjectProperty.h"
#include "../Objects/Properties/StrProperty.h"
#include "../Objects/Properties/SetProperty.h"
#include "../Objects/Properties/StructProperty.h"
#include "../Objects/Properties/TextProperty.h"
#include "../Objects/UScriptArray.h"
#include "../Objects/UScriptSet.h"
#include "../../Objects/UObject/FName.h"
#include "../../Objects/Core/i18N/FText.h"

namespace CUE4Parse::UE4::Assets::Exports
{
    using CUE4Parse::UE4::Assets::Objects::FPropertyTag;
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Objects::FScriptStruct;
    using CUE4Parse::UE4::Assets::Objects::Properties::FPropertyTagType;
    using CUE4Parse::UE4::Assets::Objects::Properties::TPropertyTagType;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using CUE4Parse::UE4::Objects::Core::i18N::FText;
    using CUE4Parse::UE4::Assets::Objects::UScriptArray;

    namespace PropertyUtil
    {
        // The payload types a property actually stores in a TPropertyTagType<T>. Everything else that is a
        // value type arrives inside a StructProperty, either as a named struct (FGuid, FSoftObjectPath, ...)
        // or as an FStructFallback the caller's type is built from -- hence the three disjoint arms below.
        template <typename T>
        inline constexpr bool IsTagScalar_v =
            std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || std::is_same_v<T, FName>;

        // C#'s [StructFallback] attribute, expressed as the ctor shape those types all have.
        template <typename T>
        inline constexpr bool IsStructFallbackType_v = std::is_constructible_v<T, const FStructFallback&>;

        // C#'s `PropertyUtil.SearchPropertyInTemplate`. Off by default, as in C#.
        inline bool SearchPropertyInTemplate = false;

        // ---- value extraction -------------------------------------------------------------------
        // One overload per shape C#'s GetValue(Type) handles. Returns false where C# returns null.

        // Defined at the bottom; declared here because the arms below call them.
        template <typename T>
        const T* NarrowerInteger(const FPropertyTagType& tag);
        bool TryAnyInteger(const FPropertyTagType& tag, int64_t& out);

        // Any scalar whose payload type matches exactly: bool, the ints, float, double, std::string, FName.
        // Enums and aggregates are excluded so they take their dedicated arms below rather than making the
        // call ambiguous (and so this arm never instantiates TPropertyTagType<SomeStruct>, which has no
        // PropValueToString and would not compile).
        template <typename T>
        std::enable_if_t<IsTagScalar_v<T>, bool> PropertyValue(const FPropertyTagType& tag, T& out)
        {
            if (const auto* exact = dynamic_cast<const TPropertyTagType<T>*>(&tag))
            {
                out = exact->Value;
                return true;
            }

            // Integer widening between the *same* signedness. A UInt32Property read into a uint64_t is the
            // same number; a signed/unsigned mix is not, and is refused, as C#'s `is T` test would.
            if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
            {
                if (const auto* v = NarrowerInteger<T>(tag)) { out = *v; return true; }
            }
            return false;
        }

        // A struct property whose payload is an FStructFallback: the shape every [StructFallback] type in
        // C# is built from. Callers ask for the fallback and construct their own struct from it.
        // StructProperty/ObjectProperty derive FPropertyTagType directly rather than TPropertyTagType<T>
        // (see the note atop StructProperty.h), so these cast to the concrete class.
        inline bool PropertyValue(const FPropertyTagType& tag, const FStructFallback*& out)
        {
            const auto* structProp = dynamic_cast<const Objects::Properties::StructProperty*>(&tag);
            if (structProp == nullptr) return false;
            const FStructFallback* fallback = structProp->Value.AsFallback();
            if (fallback == nullptr) return false;
            out = fallback;
            return true;
        }

        // An object property: the raw package index. C# would load it and hand back the UObject; callers
        // that want that go through FPackageIndex themselves, which keeps this header free of the loader.
        inline bool PropertyValue(const FPropertyTagType& tag, const FPackageIndex*& out)
        {
            const auto* objProp = dynamic_cast<const Objects::Properties::ObjectProperty*>(&tag);
            if (objProp == nullptr) return false;
            out = &objProp->Value;
            return true;
        }

        // The same by value, so `GetOrDefault<FPackageIndex>(name)` reads as it does in C#. An FPackageIndex
        // is an index plus a borrowed package pointer, so copying it costs nothing and keeps no ownership.
        inline bool PropertyValue(const FPropertyTagType& tag, FPackageIndex& out)
        {
            const FPackageIndex* value = nullptr;
            if (!PropertyValue(tag, value)) return false;
            out = *value;
            return true;
        }

        // A text property. Borrowed rather than copied: this port's FText owns a unique_ptr<FTextHistory>
        // and is move-only, so it cannot be lifted out of a const property bag. Consumers copy what they
        // need (usually Text()) while the holder is alive.
        inline bool PropertyValue(const FPropertyTagType& tag, const FText*& out)
        {
            const auto* textProp = dynamic_cast<const Objects::Properties::TextProperty*>(&tag);
            if (textProp == nullptr) return false;
            out = &textProp->Value;
            return true;
        }

        // An array property: the element list, for callers that read a TArray field.
        inline bool PropertyValue(const FPropertyTagType& tag, const UScriptArray*& out)
        {
            const auto* arrayProp = dynamic_cast<const Objects::Properties::ArrayProperty*>(&tag);
            if (arrayProp == nullptr) return false;
            out = &arrayProp->Value;
            return true;
        }

        // A set property: the raw UScriptSet, for callers that keep a TSet field as-is (e.g. Wwise's
        // GroupValueSet). C# hands back the set untouched; so does the port.
        inline bool PropertyValue(const FPropertyTagType& tag, const Objects::UScriptSet*& out)
        {
            const auto* setProp = dynamic_cast<const Objects::Properties::SetProperty*>(&tag);
            if (setProp == nullptr) return false;
            out = &setProp->Value;
            return true;
        }

        // A struct property that resolved to a concrete named struct (FGuid, FSoftObjectPath, FVector, ...):
        // FScriptStruct's switch already built the value, so this is C#'s `StructType is T t` test.
        template <typename T>
        std::enable_if_t<!IsTagScalar_v<T> && !std::is_enum_v<T> && !std::is_pointer_v<T> &&
                         !IsStructFallbackType_v<T>, bool>
        PropertyValue(const FPropertyTagType& tag, T& out)
        {
            const auto* structProp = dynamic_cast<const Objects::Properties::StructProperty*>(&tag);
            if (structProp == nullptr) return false;
            const T* value = structProp->Value.Get<T>();
            if (value == nullptr) return false;
            out = *value;
            return true;
        }

        // A struct property whose payload is the generic bag, read into a [StructFallback] type. This is what
        // C#'s `GetOrDefault<FSubtitleCue>(...)` does once its reflection has picked the ctor.
        template <typename T>
        std::enable_if_t<IsStructFallbackType_v<T>, bool> PropertyValue(const FPropertyTagType& tag, T& out)
        {
            const FStructFallback* fallback = nullptr;
            if (!PropertyValue(tag, fallback) || fallback == nullptr) return false;
            out = T(*fallback);
            return true;
        }

        // An enum-typed field. C# resolves EnumProperty's stored text ("EWwiseGroupType::Switch") back to a
        // member by reflecting over the enum; C++ has no such table, so:
        //   * a Byte/Int-backed enum field (what cooked, unversioned assets actually store) converts
        //     directly from the stored integer, which is the common case and exact;
        //   * an EnumProperty whose text after "::" is numeric converts from that;
        //   * anything else fails, and GetOrDefault falls back -- where C# would have resolved the name.
        // Generating a name table per ported enum would close the gap. TODO.
        template <typename E>
        std::enable_if_t<std::is_enum_v<E>, bool> PropertyValue(const FPropertyTagType& tag, E& out)
        {
            using U = std::underlying_type_t<E>;

            if (const auto* enumProp = dynamic_cast<const Objects::Properties::EnumProperty*>(&tag))
            {
                const std::string& text = enumProp->Value.Text();
                const size_t sep = text.rfind("::");
                const std::string member = sep == std::string::npos ? text : text.substr(sep + 2);
                if (!member.empty() && member.find_first_not_of("0123456789") == std::string::npos)
                {
                    out = static_cast<E>(static_cast<U>(std::stoll(member)));
                    return true;
                }
                return false;
            }

            // Any integer property whose value fits the enum's underlying type.
            int64_t widened = 0;
            if (PropertyValue<int64_t>(tag, widened) || TryAnyInteger(tag, widened))
            {
                out = static_cast<E>(static_cast<U>(widened));
                return true;
            }
            return false;
        }

        // ---- lookup -----------------------------------------------------------------------------

        // C#'s private TryGet(holder, name, out tag): the first property with that name.
        const FPropertyTag* FindTag(const std::vector<FPropertyTag>& properties, const std::string& name,
                                    bool ignoreCase = false);

        template <typename T, typename Holder>
        bool TryGet(const Holder& holder, const std::string& name, T& value, bool ignoreCase = false)
        {
            const FPropertyTag* prop = FindTag(holder.Properties, name, ignoreCase);
            if (prop == nullptr || prop->Tag == nullptr) return false;
            return PropertyValue(*prop->Tag, value);
        }

        template <typename T, typename Holder>
        T GetOrDefault(const Holder& holder, const std::string& name, T defaultValue = T{}, bool ignoreCase = false)
        {
            T value{};
            if (TryGet<T>(holder, name, value, ignoreCase)) return value;
            return defaultValue;
        }

        // C#'s `fallback.GetOrDefault<TStruct[]>(name, [])`: an array of [StructFallback] structs. T must be
        // constructible from a const FStructFallback&, which is exactly the shape those types have.
        // Elements that are not struct properties are skipped, where C#'s CreateArray would leave a default.
        template <typename T, typename Holder>
        std::vector<T> GetStructArray(const Holder& holder, const std::string& name)
        {
            std::vector<T> result;
            const UScriptArray* array = nullptr;
            if (!TryGet<const UScriptArray*>(holder, name, array) || array == nullptr) return result;

            result.reserve(array->Properties.size());
            for (const auto& element : array->Properties)
            {
                if (element == nullptr) continue;
                const FStructFallback* fallback = nullptr;
                if (PropertyValue(*element, fallback) && fallback != nullptr) result.emplace_back(*fallback);
            }
            return result;
        }

        // C#'s `GetOrDefault<T[]>(name, [])` for any element type PropertyValue handles: the scalars, the
        // named structs (FGuid[]), FPackageIndex[], and the [StructFallback] types. Elements whose stored
        // shape does not convert are skipped, where C#'s CreateArray would leave a default in place.
        // (GetStructArray below stays as its own function: it does not require a default-constructible T.)
        template <typename T, typename Holder>
        std::vector<T> GetArray(const Holder& holder, const std::string& name)
        {
            std::vector<T> result;
            const UScriptArray* array = nullptr;
            if (!TryGet<const UScriptArray*>(holder, name, array) || array == nullptr) return result;

            result.reserve(array->Properties.size());
            for (const auto& element : array->Properties)
            {
                if (element == nullptr) continue;
                T value{};
                if (PropertyValue(*element, value)) result.push_back(std::move(value));
            }
            return result;
        }

        // C#'s Get<T>: same lookup, but throws where GetOrDefault would fall back.
        template <typename T, typename Holder>
        T Get(const Holder& holder, const std::string& name, bool ignoreCase = false)
        {
            T value{};
            if (!TryGet<T>(holder, name, value, ignoreCase))
                throw std::runtime_error("Couldn't get property '" + name + "' of the requested type");
            return value;
        }

        // ---- internals --------------------------------------------------------------------------

        // Widening helper for PropertyValue: finds a stored integer of the same signedness that fits in T.
        template <typename T>
        const T* NarrowerInteger(const FPropertyTagType& tag)
        {
            static thread_local T scratch{};
            const auto tryOne = [&](auto probe) -> bool
            {
                using U = decltype(probe);
                if constexpr (std::is_signed_v<U> == std::is_signed_v<T> && sizeof(U) <= sizeof(T))
                {
                    if (const auto* p = dynamic_cast<const TPropertyTagType<U>*>(&tag))
                    {
                        scratch = static_cast<T>(p->Value);
                        return true;
                    }
                }
                return false;
            };
            if (tryOne(int8_t{}) || tryOne(int16_t{}) || tryOne(int32_t{}) || tryOne(int64_t{}) ||
                tryOne(uint8_t{}) || tryOne(uint16_t{}) || tryOne(uint32_t{}) || tryOne(uint64_t{}))
                return &scratch;
            return nullptr;
        }
    }
}
