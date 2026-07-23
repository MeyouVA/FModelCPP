// Ported from CUE4Parse/MappingsProvider/TypeMappings.cs
// The parsed mappings for one game: struct schemas by name and enum value->member tables by enum name.
//
// Deliberate differences from C#:
//   * C#'s Dictionary comparer is baked in at construction; here the Types map takes a runtime
//     Utils::StringComparer (the parser passes OrdinalIgnoreCase unless told otherwise, like C#).
//   * Structs are held by shared_ptr (C# leans on GC): a Struct carries a back-pointer to its owning
//     TypeMappings (Context), so a TypeMappings must stay at a stable address — providers hold it by
//     shared_ptr and hand out raw pointers.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "../Utils/StringComparer.h"

namespace CUE4Parse::MappingsProvider
{
    class Struct;

    class TypeMappings
    {
    public:
        std::map<std::string, std::shared_ptr<Struct>, Utils::StringComparer> Types;
        std::map<std::string, std::map<int64_t, std::string>> Enums;

        explicit TypeMappings(Utils::StringComparer comparer = Utils::StringComparer::Ordinal())
            : Types(comparer) {}

        TypeMappings(const TypeMappings&) = delete;
        TypeMappings& operator=(const TypeMappings&) = delete;
    };
}
