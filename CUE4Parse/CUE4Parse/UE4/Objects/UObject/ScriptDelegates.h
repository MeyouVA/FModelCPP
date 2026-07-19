// Ported from CUE4Parse/UE4/Objects/UObject/ScriptDelegates.cs
// FScriptDelegate (a bound object + function name) and FMulticastScriptDelegate (an ordered invocation list
// of FScriptDelegate). Read from an FAssetArchive.
//
// Deliberate difference: C# has no ToString override on these (they'd print the .NET type name); the port
// gives them readable ToString forms since the property ToString path uses them.
#pragma once

#include <string>
#include <vector>

#include "FName.h"
#include "ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class FScriptDelegate
    {
    public:
        // The object bound to this delegate (null index if no object is bound).
        FPackageIndex Object;
        // Name of the function to call on the bound object.
        FName FunctionName;

        FScriptDelegate() = default;
        explicit FScriptDelegate(Assets::Readers::FAssetArchive& Ar);
        FScriptDelegate(FPackageIndex object, FName functionName)
            : Object(std::move(object)), FunctionName(std::move(functionName)) {}

        std::string ToString() const { return FunctionName.Text(); }
    };

    class FMulticastScriptDelegate
    {
    public:
        // Ordered list of functions to invoke when Broadcast is called.
        std::vector<FScriptDelegate> InvocationList;

        FMulticastScriptDelegate() = default;
        explicit FMulticastScriptDelegate(Assets::Readers::FAssetArchive& Ar);
        explicit FMulticastScriptDelegate(std::vector<FScriptDelegate> invocationList)
            : InvocationList(std::move(invocationList)) {}

        std::string ToString() const { return "[" + std::to_string(InvocationList.size()) + " invocations]"; }
    };
}
