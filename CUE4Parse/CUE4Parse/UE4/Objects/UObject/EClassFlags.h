// Ported from CUE4Parse/UE4/Objects/UObject/EClassFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::UObject
{
    // [Flags]
    enum EClassFlags : uint32_t
    {
        CLASS_None                           = 0x00000000u,
        CLASS_Abstract                       = 0x00000001u, // Class is abstract and can't be instantiated directly.
        CLASS_DefaultConfig                  = 0x00000002u, // Save object configuration only to Default INIs.
        CLASS_Config                         = 0x00000004u, // Load object configuration at construction time.
        CLASS_Transient                      = 0x00000008u, // This object type can't be saved.
        CLASS_Optional                       = 0x00000010u, // May not be available in certain context.
        CLASS_MatchedSerializers             = 0x00000020u,
        CLASS_ProjectUserConfig              = 0x00000040u, // Config saved to Project/User*.ini.
        CLASS_Native                         = 0x00000080u, // Class is a native class.
        CLASS_NoExport                       = 0x00000100u, // Don't export to C++ header.
        CLASS_NotPlaceable                   = 0x00000200u, // Do not allow users to create in the editor.
        CLASS_PerObjectConfig                = 0x00000400u, // Handle object config per-object.
        CLASS_ReplicationDataIsSetUp         = 0x00000800u,
        CLASS_EditInlineNew                  = 0x00001000u, // Constructed from editinline New button.
        CLASS_CollapseCategories             = 0x00002000u, // Display properties without categories.
        CLASS_Interface                      = 0x00004000u, // Class is an interface.
        CLASS_CustomConstructor              = 0x00008000u,
        CLASS_Const                          = 0x00010000u, // all properties and functions are const.
        CLASS_NeedsDeferredDependencyLoading = 0x00020000u,
        CLASS_CompiledFromBlueprint          = 0x00040000u, // Created from blueprint source material.
        CLASS_MinimalAPI                     = 0x00080000u,
        CLASS_RequiredAPI                    = 0x00100000u,
        CLASS_DefaultToInstanced             = 0x00200000u,
        CLASS_TokenStreamAssembled           = 0x00400000u,
        CLASS_HasInstancedReference          = 0x00800000u,
        CLASS_Hidden                         = 0x01000000u,
        CLASS_Deprecated                     = 0x02000000u,
        CLASS_HideDropDown                   = 0x04000000u,
        CLASS_GlobalUserConfig               = 0x08000000u,
        CLASS_Intrinsic                      = 0x10000000u,
        CLASS_Constructed                    = 0x20000000u,
        CLASS_ConfigDoNotCheckDefaults       = 0x40000000u,
        CLASS_NewerVersionExists             = 0x80000000u,
    };
}
