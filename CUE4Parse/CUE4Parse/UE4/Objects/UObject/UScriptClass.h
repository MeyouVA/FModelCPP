// Ported from CUE4Parse/UE4/Objects/UObject/UScriptClass.cs
// Not an engine class: a UClass stand-in for a native (code-defined) class known only by name — what a
// script import or a mappings struct name resolves to. Never registered in ObjectTypeRegistry (C#
// [SkipObjectRegistration]).
//
// Deliberate difference from C#: the USharpClass/UPythonClass/UASClass aliases are omitted until a
// consumer exists.
#pragma once

#include <string>
#include <utility>

#include "UClass.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    class UScriptClass : public UClass
    {
    public:
        explicit UScriptClass(std::string className)
        {
            Name = std::move(className);
        }
    };
}
