// Ported from CUE4Parse/MappingsProvider/MappingsSchema.cs
// The mappings schema: a Struct (name + super + indexed property table), its PropertyInfo entries and the
// recursive PropertyType descriptor. SerializedStruct adapts a loaded UStruct (its FProperty children)
// into the same shape so unversioned deserialization can consume either source.
//
// Deliberate differences from C#:
//   * C#'s Lazy<Struct?> Super becomes an on-first-use resolve with a cached result (Super()); a
//     SerializedStruct may *own* the super it builds (C# news one up per resolve under GC), so the cache
//     also keeps a shared_ptr for that case.
//   * TryGetValue(i, out info) returns `const PropertyInfo*` (null on a miss).
//   * PropertyType's UStruct*/UEnum* back-references are non-owning (the objects belong to their packages).
//   * PropertyInfo.Clone() (C# MemberwiseClone) is the implicit copy ctor; MappingType stays shared.
//   * SerializedStruct's super resolution loads through FPackageIndex.Owner->ResolvePackageIndex (this
//     port's FPackageIndex has no Load<T> of its own).
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "TypeMappings.h"

namespace CUE4Parse::UE4::Objects::UObject { class UStruct; class UEnum; class FProperty; }

namespace CUE4Parse::MappingsProvider
{
    class PropertyType
    {
    public:
        std::string Type;
        std::optional<std::string> StructType;
        std::shared_ptr<PropertyType> InnerType;
        std::shared_ptr<PropertyType> ValueType;
        std::optional<std::string> EnumName;
        std::optional<bool> IsEnumAsByte;
        std::optional<bool> Bool;
        const UE4::Objects::UObject::UStruct* Struct = nullptr;
        const UE4::Objects::UObject::UEnum* Enum = nullptr;

        explicit PropertyType(std::string type,
                              std::optional<std::string> structType = std::nullopt,
                              std::shared_ptr<PropertyType> innerType = nullptr,
                              std::shared_ptr<PropertyType> valueType = nullptr,
                              std::optional<std::string> enumName = std::nullopt,
                              std::optional<bool> isEnumAsByte = std::nullopt,
                              std::optional<bool> b = std::nullopt)
            : Type(std::move(type)), StructType(std::move(structType)), InnerType(std::move(innerType)),
              ValueType(std::move(valueType)), EnumName(std::move(enumName)), IsEnumAsByte(isEnumAsByte), Bool(b) {}

        // C#'s PropertyType(FProperty): derives the descriptor from a loaded reflection property
        // (SerializedStruct path). Defined in MappingsSchema.cpp.
        explicit PropertyType(const UE4::Objects::UObject::FProperty& prop);
    };

    class PropertyInfo
    {
    public:
        int Index = 0;
        std::string Name;
        std::optional<int> ArraySize;
        std::shared_ptr<PropertyType> MappingType;

        PropertyInfo(int index, std::string name, std::shared_ptr<PropertyType> mappingType,
                     std::optional<int> arraySize = std::nullopt)
            : Index(index), Name(std::move(name)), ArraySize(arraySize), MappingType(std::move(mappingType)) {}

        std::string ToString() const
        {
            return std::to_string(Index + 1) + "/" + (ArraySize ? std::to_string(*ArraySize) : "") + " -> " + Name;
        }
    };

    class Struct
    {
    public:
        const TypeMappings* Context = nullptr;
        std::string Name;
        std::optional<std::string> SuperType;
        std::map<int, std::shared_ptr<PropertyInfo>> Properties;
        int PropertyCount = 0;

        Struct(const TypeMappings* context, std::string name, int propertyCount)
            : Context(context), Name(std::move(name)), PropertyCount(propertyCount) {}

        Struct(const TypeMappings* context, std::string name, std::optional<std::string> superType,
               std::map<int, std::shared_ptr<PropertyInfo>> properties, int propertyCount)
            : Context(context), Name(std::move(name)), SuperType(std::move(superType)),
              Properties(std::move(properties)), PropertyCount(propertyCount) {}

        virtual ~Struct() = default;

        // C#'s Lazy<Struct?> Super: resolved on first call, cached (including a cached miss).
        Struct* Super() const
        {
            if (!_superResolved)
            {
                _superResolved = true;
                _super = ResolveSuper();
            }
            return _super;
        }

        // C#'s TryGetValue(i, out info): the property at schema index i, walking supers past this struct's
        // own PropertyCount. Null on a miss.
        const PropertyInfo* TryGetValue(int i) const
        {
            const auto it = Properties.find(i);
            if (it == Properties.end())
            {
                if (i >= PropertyCount && Super() != nullptr)
                    return Super()->TryGetValue(i - PropertyCount);
                return nullptr;
            }
            return it->second.get();
        }

        int CountProperties(bool includeSuper) const
        {
            int total = 0;
            for (const Struct* current = this; current != nullptr;
                 current = includeSuper ? current->Super() : nullptr)
            {
                total += current->PropertyCount;
            }
            return total;
        }

    protected:
        // Base resolution: look SuperType up in the owning TypeMappings (C#'s base Lazy). Overridden by
        // SerializedStruct.
        virtual Struct* ResolveSuper() const;

        // A super this struct built (and therefore owns) during resolution — SerializedStruct's
        // "new SerializedStruct(...)" case; the base class only ever points into Context.
        mutable std::shared_ptr<Struct> _superOwned;

    private:
        mutable Struct* _super = nullptr;
        mutable bool _superResolved = false;
    };

    // A Struct built from a loaded UStruct's FProperty children (C#'s SerializedStruct).
    class SerializedStruct : public Struct
    {
    public:
        SerializedStruct(const TypeMappings* context, const UE4::Objects::UObject::UStruct& struc);

    protected:
        Struct* ResolveSuper() const override;

    private:
        const UE4::Objects::UObject::UStruct* _struct = nullptr;
    };
}
