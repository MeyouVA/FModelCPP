// Ported from CUE4Parse/UE4/Assets/Objects/Properties/FPropertyTagType.cs
// The value side of a tagged property: an abstract FPropertyTagType with a typed TPropertyTagType<T>
// subclass, plus the factory that dispatches a property-type name to a concrete reader.
//
// Deliberate differences from C#:
//   * The reflection-driven GetValue/GetValue<T> machinery (which converts a property value to an arbitrary
//     C# type for the JSON/consumer layer) is not ported — it relies on .NET reflection. ToString() is kept.
//   * GenericValue (object?) has no clean C++ equivalent and is omitted; concrete subclasses expose their
//     typed Value directly. Consumers dynamic_cast to the concrete property type.
//   * Only a scalar subset of property types is dispatched so far (bool/byte/ints/floats/name/str). The
//     container/object/struct/text/enum/soft/delegate types return nullptr (unknown) until ported. TODO.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects { class FPropertyTagData; }
namespace CUE4Parse::UE4::Objects::UObject { struct FName; }

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using Readers::FAssetArchive;

    // Value formatters used by TPropertyTagType<T>::ToString (defined in the .cpp). One per scalar payload.
    std::string PropValueToString(bool v);
    std::string PropValueToString(signed char v);
    std::string PropValueToString(unsigned char v);
    std::string PropValueToString(short v);
    std::string PropValueToString(unsigned short v);
    std::string PropValueToString(int v);
    std::string PropValueToString(unsigned int v);
    std::string PropValueToString(long long v);
    std::string PropValueToString(unsigned long long v);
    std::string PropValueToString(float v);
    std::string PropValueToString(double v);
    std::string PropValueToString(const std::string& v);
    std::string PropValueToString(const CUE4Parse::UE4::Objects::UObject::FName& v);

    enum class ReadType : uint8_t
    {
        ZERO,
        NORMAL,
        MAP,
        ARRAY,
        OPTIONAL,
        RAW,
    };

    class FPropertyTagType
    {
    public:
        virtual ~FPropertyTagType() = default;

        // The concrete property class name (e.g. "IntProperty"), matching C#'s GetType().Name in ToString.
        virtual const char* TypeName() const = 0;
        virtual std::string ToString() const = 0;

        // Factory: reads the value for `propertyType`. Returns null for a type not yet supported (C# logs
        // and returns null in that case too).
        static std::unique_ptr<FPropertyTagType> ReadPropertyTagType(
            FAssetArchive& Ar, const std::string& propertyType, const FPropertyTagData* tagData,
            ReadType type, int size = 0);
    };

    // Typed property value. Concrete scalar properties derive from this and set Value in their ctor.
    template <typename T>
    class TPropertyTagType : public FPropertyTagType
    {
    public:
        T Value{};

        std::string ToString() const override
        {
            return PropValueToString(Value) + " (" + TypeName() + ")";
        }
    };
}
