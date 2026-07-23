// Ported from CUE4Parse/MappingsProvider/ITypeMappingsProvider.cs and AbstractTypeMappingsProvider.cs.
// The provider hands out the TypeMappings for the current game and can (re)load them from a path or bytes.
//
// Deliberate differences from C#:
//   * MappingsForGame returns a raw pointer into provider-owned storage (C# nullable property).
//   * The C# StringComparer? parameter becomes an optional Utils::StringComparer (nullopt = the parser's
//     OrdinalIgnoreCase default).
//   * AbstractTypeMappingsProvider (which only re-abstracts the interface) is folded into this header.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "TypeMappings.h"

namespace CUE4Parse::MappingsProvider
{
    class ITypeMappingsProvider
    {
    public:
        virtual ~ITypeMappingsProvider() = default;

        virtual const TypeMappings* MappingsForGame() const = 0;

        virtual void Load(const std::string& path, std::optional<Utils::StringComparer> comparer = std::nullopt) = 0;
        virtual void Load(const std::vector<uint8_t>& bytes, std::optional<Utils::StringComparer> comparer = std::nullopt) = 0;

        virtual void Reload() = 0;
    };

    class AbstractTypeMappingsProvider : public ITypeMappingsProvider
    {
    };
}
