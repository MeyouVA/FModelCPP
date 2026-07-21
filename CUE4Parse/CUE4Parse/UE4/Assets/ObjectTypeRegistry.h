// Ported from CUE4Parse/UE4/Assets/ObjectTypeRegistry.cs.
//
// Maps a serialized class name (e.g. "StringTable") to a concrete UObject subclass so ConstructObject can
// build the right type. C# discovers every UObject subclass by reflecting over the assembly; C++ has no
// reflection, so the ported engine export types register a factory by hand (see RegisterEngineTypes in the
// .cpp). C#'s RegisterClass strips a leading 'U'/'A' from the type name ("UStringTable" -> "StringTable"),
// which is exactly the serialized name; we register that name directly.
#pragma once

#include <functional>
#include <memory>
#include <string>

namespace CUE4Parse::UE4::Assets::Exports { class UObject; }

namespace CUE4Parse::UE4::Assets
{
    class ObjectTypeRegistry
    {
    public:
        // A factory that builds a fresh, empty instance of a registered export type.
        using Factory = std::function<std::unique_ptr<Exports::UObject>()>;

        // Registers (or replaces) the factory for a serialized class name.
        static void RegisterClass(const std::string& serializedName, Factory factory);

        // The factory for `serializedName` (falling back to the name with a trailing "_C" stripped, as in
        // C#), or an empty std::function if none is registered.
        static Factory Get(const std::string& serializedName);
    };
}
