// Ported from CUE4Parse/UE4/Assets/Exports/EObjectFlags.cs
// Flags describing an object instance. Only referenced (so far) by FNameEntrySerialized's pre-UE4
// flag read; the full enum is ported for fidelity.
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports
{
    // [Flags]
    enum EObjectFlags : uint32_t
    {
        // Do not add new flags unless they truly belong here. There are alternatives.
        RF_NoFlags                      = 0x00000000, // No flags, used to avoid a cast

        // This first group of flags mostly has to do with what kind of object it is.
        RF_Public                       = 0x00000001, // Object is visible outside its package.
        RF_Standalone                   = 0x00000002, // Keep object around for editing even if unreferenced.
        RF_MarkAsNative                 = 0x00000004, // Object (UField) will be marked as native on construction.
        RF_Transactional                = 0x00000008, // Object is transactional.
        RF_ClassDefaultObject           = 0x00000010, // This object is its class's default object
        RF_ArchetypeObject              = 0x00000020, // This object is a template for another object.
        RF_Transient                    = 0x00000040, // Don't save object.

        // This group of flags is primarily concerned with garbage collection.
        RF_MarkAsRootSet                = 0x00000080, // Object will be marked as root set on construction.
        RF_TagGarbageTemp               = 0x00000100, // Temp user flag for utilities that need the GC.

        // The group of flags tracks the stages of the lifetime of a uobject
        RF_NeedInitialization           = 0x00000200, // This object has not completed its initialization process.
        RF_NeedLoad                     = 0x00000400, // During load, indicates object needs loading.
        RF_KeepForCooker                = 0x00000800, // Keep this object during GC because it's still used by the cooker
        RF_NeedPostLoad                 = 0x00001000, // Object needs to be postloaded.
        RF_NeedPostLoadSubobjects       = 0x00002000, // During load, still needs to instance subobjects.
        RF_NewerVersionExists           = 0x00004000, // Object consigned to oblivion; a newer version exists.
        RF_BeginDestroyed               = 0x00008000, // BeginDestroy has been called on the object.
        RF_FinishDestroyed              = 0x00010000, // FinishDestroy has been called on the object.

        // Misc. Flags
        RF_BeingRegenerated             = 0x00020000, // Flagged on UObjects used to create UClasses while regenerating.
        RF_DefaultSubObject             = 0x00040000, // Flagged on subobjects that are defaults
        RF_WasLoaded                    = 0x00080000, // Flagged on UObjects that were loaded
        RF_TextExportTransient          = 0x00100000, // Do not export object to text form.
        RF_LoadCompleted                = 0x00200000, // Object has been completely serialized at least once.
        RF_InheritableComponentTemplate = 0x00400000, // Archetype of the object can be in its super class
        RF_DuplicateTransient           = 0x00800000, // Object should not be included in any type of duplication.
        RF_StrongRefOnFrame             = 0x01000000, // Persistent-frame references handled as strong ones.
        RF_NonPIEDuplicateTransient     = 0x02000000, // Not included for duplication unless duplicating for PIE.
        RF_Dynamic                      = 0x04000000, // Field Only. Dynamic field.
        RF_WillBeLoaded                 = 0x08000000, // Constructed during load and will be loaded shortly
    };
}
