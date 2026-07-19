// Ported from CUE4Parse/UE4/IUStruct.cs
// Empty marker interface tagging "Unreal struct" value types. Kept as an empty base so that
// deriving value structs (FGuid, FDateTime, ...) stay trivially copyable and standard-layout
// (empty-base optimization) — a prerequisite for reading them via FArchive::Read<T>.
#pragma once

namespace CUE4Parse::UE4
{
    struct IUStruct
    {
    };
}
