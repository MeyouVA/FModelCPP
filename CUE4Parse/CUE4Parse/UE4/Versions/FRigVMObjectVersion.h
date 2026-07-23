// Ported from CUE4Parse/UE4/Versions/FRigVMObjectVersion.cs
// Custom serialization version for changes made in Dev-Anim stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FRigVMObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded,

            // ControlRig & RigVMHost compute and checks VM Hash
            AddedVMHashChecks,

            // Predicates added to execute operations
            PredicatesAddedToExecuteOps,

            // Storing paths to user defined structs map
            VMStoringUserDefinedStructMap,

            // Storing paths to user defined enums map
            VMStoringUserDefinedEnumMap,

            // Storing paths to user defined enums map
            HostStoringUserDefinedData,

            // VM Memory Storage Struct serialized
            VMMemoryStorageStructSerialized,

            // VM Memory Storage Defaults generated at VM
            VMMemoryStorageDefaultsGeneratedAtVM,

            // VM Bytecode Stores the Public Context Path
            VMBytecodeStorePublicContextPath,

            // Removing unused tooltip property from frunction header
            VMRemoveTooltipFromFunctionHeader,

            // Removing library node FSoftObjectPath from FRigVMGraphFunctionIdentifier
            RemoveLibraryNodeReferenceFromFunctionIdentifier,

            // Adding variant struct to function identifier
            AddVariantToFunctionIdentifier,

            // Adding variant to every RigVM asset
            AddVariantToRigVMAssets,

            // Storing user interface layout within function header
            FunctionHeaderStoresLayout,

            // Storing user interface relevant pin index in category
            FunctionHeaderLayoutStoresPinIndexInCategory,

            // Storing user interface relevant category expansion
            FunctionHeaderLayoutStoresCategoryExpansion,

            // Storing function graph collapse node content as part of the header
            RigVMSaveSerializedGraphInGraphFunctionDataAsByteArray,

            // VM Bytecode Stores the Public Context Path as a FTopLevelAssetPath
            VMBytecodeStorePublicContextPathAsTopLevelAssetPath,

            // Serialized instruction offsets are now int32 rather than uint16, NumBytes has been removed
            // from RigVMCopyOp
            ByteCodeCleanup,

            // The VM stores a local snapshot registry to use in cooked environments instead of the shared global registry
            LocalizedRegistry,

            // The VM stores a relative seek offset to be able to skip the registry during load
            LocalizedRegistryWithRelativeSeekOffset,

            // Function arguments can now represent an input variable (an external variable passed into a function)
            FunctionArgumentCanRepresentInputVariable,

            // Object archive is now storing the version container
            ObjectArchiveVersionContainerSerialization,

            // Debug operand mapping simplified and moved to context only
            DebugOperandMappingSimplified,

            // Introduction of callables to the rigvm bytecode
            RigVMCallables,

            // Referencing variables through Guids
            GuidForVariables,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1,
        };

        inline const FGuid GUID(0xDC49959B, 0x53C04DE7, 0x9156EA88, 0x5E7C5D39);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game == GAME_Aion2) return VMMemoryStorageDefaultsGeneratedAtVM;
            if (game < GAME_UE5_3) return static_cast<Type>(-1);
            if (game < GAME_UE5_4) return PredicatesAddedToExecuteOps;
            if (game < GAME_UE5_5) return VMRemoveTooltipFromFunctionHeader;
            if (game < GAME_UE5_6) return FunctionHeaderLayoutStoresCategoryExpansion;
            if (game < GAME_UE5_7) return ByteCodeCleanup;
            if (game < GAME_UE5_8) return LocalizedRegistry;
            return LatestVersion;
        }
    }
}
