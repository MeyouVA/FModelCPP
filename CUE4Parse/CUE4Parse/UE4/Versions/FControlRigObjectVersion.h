// Ported from CUE4Parse/UE4/Versions/FControlRigObjectVersion.cs
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
    namespace FControlRigObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded,

            // Added execution pins and removed hierarchy ref pins
            RemovalOfHierarchyRefPins,

            // Refactored operators to store FCachedPropertyPath instead of string
            OperatorsStoringPropertyPaths,

            // Introduced new RigVM as a backend
            SwitchedToRigVM,

            // Added a new transform as part of the control
            ControlOffsetTransform,

            // Using a cache data structure for key indices now
            RigElementKeyCache,

            // Full variable support
            BlueprintVariableSupport,

            // Hierarchy V2.0
            RigHierarchyV2,

            // RigHierarchy to support multi component parent constraints
            RigHierarchyMultiParentConstraints,

            // RigHierarchy now supports space favorites per control
            RigHierarchyControlSpaceFavorites,

            // RigHierarchy now stores min and max values as float storages
            StorageMinMaxValuesAsFloatStorage,

            // RenameGizmoToShape
            RenameGizmoToShape,

            // BoundVariableWithInjectionNode
            BoundVariableWithInjectionNode,

            // Switch limit control over to per channel limits
            PerChannelLimits,

            // Removed the parent cache for multi parent elements
            RemovedMultiParentParentCache,

            // Deprecation of parameters
            RemoveParameters,

            // Added rig curve element value state flag
            CurveElementValueStateFlag,

            // Added the notion of a per control animation type
            ControlAnimationType,

            // Added preferred permutation for templates
            TemplatesPreferredPermutatation,

            // Added preferred euler angles to controls
            PreferredEulerAnglesForControls,

            // Added rig hierarchy element metadata
            HierarchyElementMetadata,

            // Converted library nodes to templates
            LibraryNodeTemplates,

            // Controls to be able specify space switch targets
            RestrictSpaceSwitchingForControls,

            // Controls to be able specify which channels should be visible in sequencer
            ControlTransformChannelFiltering,

            // Store function information (and compilation data) in blueprint generated class
            StoreFunctionsInGeneratedClass,

            // Hierarchy storing previous names
            RigHierarchyStoringPreviousNames,

            // Control supporting preferred rotation order
            RigHierarchyControlPreferredRotationOrder,

            // Last bit required for Control supporting preferred rotation order
            RigHierarchyControlPreferredRotationOrderFlag,

            // Element metadata is now stored on URigHierarchy, rather than FRigBaseElement
            RigHierarchyStoresElementMetadata,

            // Add type (primary, secondary) and optional bool to FRigConnectorSettings
            ConnectorsWithType,

            // Add parent key to control rig pose
            RigPoseWithParentKey,

            // Physics solvers stored on hierarchy
            ControlRigStoresPhysicsSolvers,

            // Moved the element storage into separate buffers
            RigHierarchyIndirectElementStorage,

            // Compress the rig hierarchy when storing to disk
            RigHierarchyCompressElements,

            // Added the notion of components to the rig hierarchy
            RigHierarchyStoresComponents,

            // Improve transform compactness when serializing the hierarchy
            RigHierarchyCompactTransformSerialization,

            // Connectors to support arrays
            RigHierarchyArrayConnectors,

            // Parent constraints offering a display label
            RigHierarchyParentContraintWithLabel,

            // Previous name and parent maps serialized as FRigHierarchyKey
            RigHierarchyPreviousNameAndParentMapUsingHierarchyKey,

            // New setting for connectors to optionally specify their use only during post construction
            RigHierarchyPostConstructionConnectors,

            // Overrides store TOC data for properties to solidify loading of data when the definition has changed
            OverridesStoreTOCDataForProperties,

            // Overrides the skip offset as int64 - previous versions stored it as int32
            OverridesStoreDatSkipOffsetAsInt64,

            // Overrides now only store the path to the leaf as well as the details about the leaf property
            OverridesStorePathAndLeafPropertyOnly,

            // Overrides now only store the hash for validation, not the size since size can change without changing the payload (containers), also starts including hash for maps and sets
            OverridesStoreLeafPropertyHashOnly,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1,
        };

        inline const FGuid GUID(0xA7820CFB, 0x20A74359, 0x8C542C14, 0x9623CF50);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_23) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_25) return OperatorsStoringPropertyPaths;
            if (game < GAME_UE4_26) return SwitchedToRigVM;
            if (game < GAME_UE5_0) return BlueprintVariableSupport;
            if (game < GAME_UE5_1) return PerChannelLimits;
            if (game < GAME_UE5_2) return LibraryNodeTemplates;
            if (game < GAME_UE5_3) return RigHierarchyStoringPreviousNames;
            if (game < GAME_UE5_4) return RigHierarchyControlPreferredRotationOrderFlag;
            if (game < GAME_UE5_5) return RigPoseWithParentKey;
            if (game < GAME_UE5_6) return RigHierarchyIndirectElementStorage;
            if (game < GAME_UE5_7) return RigHierarchyPreviousNameAndParentMapUsingHierarchyKey;
            if (game < GAME_UE5_8) return OverridesStoreTOCDataForProperties;
            return LatestVersion;
        }
    }
}
