// Ported from CUE4Parse/UE4/Wwise/Objects/ICAkIndexable.cs
#pragma once

namespace CUE4Parse::UE4::Wwise::Objects
{
    // A marker interface -- C# declares it with no members at all. It exists so the HIRC containers that
    // own an ID can be collected together; nothing dispatches through it.
    class ICAkIndexable
    {
    public:
        virtual ~ICAkIndexable() = default;
    };
}
