#pragma once
// Ported from FModel/Enums.cs.
//
// C#'s [Description] attribute (read by the settings UI through EnumExtensions.GetDescription) has no direct
// C++ equivalent. It is reproduced here as an overload set: one `description(E)` function per enum, resolved
// by argument type. The same convention is used for the CUE4Parse-side enums, where it is spelled
// `Description(E)` and returns const char* so that library stays free of Qt.
//
// statusKindName() reproduces C#'s enum.ToString() (the member name), which FStatus uses to build its label.
//
// EAssetCategory is spelled with a literal base (0x00010000) where C# writes
// AssetCategoryExtensions.CategoryBase: C# tolerates the cycle between Enums.cs and AssetCategoryExtensions.cs,
// C++ headers do not. AssetCategoryExtensions.h keeps the named constant and static_asserts the two agree, so
// the duplication cannot drift.

#include <cstdint>

#include <QString>

namespace FModel
{
    enum class EBuildKind
    {
        Debug,
        Release,
        Unknown
    };

    enum class EErrorKind
    {
        Ignore,
        Restart,
        ResetSettings
    };

    enum class SettingsOut
    {
        ReloadLocres,
        ReloadMappings
    };

    enum class EStatusKind
    {
        Ready,       // ready
        Configuring, // waiting for user input
        Loading,     // doing stuff
        Stopping,    // trying to stop
        Stopped,     // stopped
        Failed,      // crashed
        Completed    // worked
    };

    enum class EEndpointType
    {
        Aes,
        Mapping
    };

    enum class EBulkType
    {
        None       = 0,
        Auto       = 1 << 0,
        Properties = 1 << 1,
        Textures   = 1 << 2,
        Meshes     = 1 << 3,
        Animations = 1 << 4,
        Audio      = 1 << 5,
        Code       = 1 << 6,
        Raw        = 1 << 7,
    };

    // C# gets `|`, `&` and HasFlag on any enum for free; a C++ enum class has no operators at all, so the
    // three EBulkType uses (RightClickMenuCommand combines a type with Auto, the extractors test for it)
    // are spelled out here.
    inline constexpr EBulkType operator|(EBulkType a, EBulkType b)
    { return static_cast<EBulkType>(static_cast<int>(a) | static_cast<int>(b)); }
    inline constexpr EBulkType operator&(EBulkType a, EBulkType b)
    { return static_cast<EBulkType>(static_cast<int>(a) & static_cast<int>(b)); }
    inline constexpr bool hasFlag(EBulkType value, EBulkType flag)
    { return (static_cast<int>(value) & static_cast<int>(flag)) == static_cast<int>(flag); }

    enum class EUnluacMode
    {
        Decompile,
        Disassemble,
    };

    enum class EAesReload
    {
        Always,
        Never,
        OncePerDay
    };

    enum class EDiscordRpc
    {
        Always,
        Never
    };

    enum class ELoadingMode
    {
        Multiple,
        All,
        AllButNew,
        AllButModified,
        AllButPatched,
    };

    enum class ECompressedAudio
    {
        PlayDecompressed,
        PlayCompressed
    };

    // The asset-tree categories. Values are structured: the high 16 bits identify the base category and the
    // low 16 bits a leaf within it, which is what AssetCategoryExtensions' helpers key off.
    enum class EAssetCategory : uint32_t
    {
        All = 0x00010000u + (0u << 16),
        Blueprints = 0x00010000u + (1u << 16),
            BlueprintGeneratedClass = Blueprints + 1,
            WidgetBlueprintGeneratedClass = Blueprints + 2,
            AnimBlueprintGeneratedClass = Blueprints + 3,
            RigVMBlueprintGeneratedClass = Blueprints + 4,
            UserDefinedEnum = Blueprints + 5,
            UserDefinedStruct = Blueprints + 6,
            //Metadata
            Blueprint = Blueprints + 8,
            CookedMetaData = Blueprints + 9,
        Mesh = 0x00010000u + (2u << 16),
            StaticMesh = Mesh + 1,
            SkeletalMesh = Mesh + 2,
            CustomizableObject = Mesh + 3,
            NaniteDisplacedMesh = Mesh + 4,
        Texture = 0x00010000u + (3u << 16),
        Materials = 0x00010000u + (4u << 16),
            Material = Materials + 1,
            MaterialEditorData = Materials + 2,
            MaterialFunction = Materials + 3,
            MaterialFunctionEditorData = Materials + 4,
            MaterialParameterCollection = Materials + 5,
        Animation = 0x00010000u + (5u << 16),
            Skeleton = Animation + 1,
            Rig = Animation + 2,
        Level = 0x00010000u + (6u << 16),
            World = Level + 1,
            BuildData = Level + 2,
            LevelSequence = Level + 3,
            Foliage = Level + 4,
        Data = 0x00010000u + (7u << 16),
            ItemDefinitionBase = Data + 1,
            CurveBase = Data + 2,
            PhysicsAsset = Data + 3,
            ObjectRedirector = Data + 4,
            PhysicalMaterial = Data + 5,
            ByteCode = Data + 6,
        Media = 0x00010000u + (8u << 16),
            Audio = Media + 1,
            Video = Media + 2,
            Font = Media + 3,
            SoundBank = Media + 4,
            AudioEvent = Media + 5,
        Particle = 0x00010000u + (9u << 16),
        GameSpecific = 0x00010000u + (10u << 16),
            Borderlands = GameSpecific + 1,
            Aion2 = GameSpecific + 2,
            RocoKingdomWorld = GameSpecific + 3,
            DeltaForce = GameSpecific + 4,
            LegoBatman = GameSpecific + 5,
    };

    enum class EIconStyle
    {
        Default,
        NoBackground,
        NoText,
        Flat,
        Cataba,
        // CommunityMade — commented out in the C# source too.
    };

    inline QString description(EAesReload value)
    {
        switch (value)
        {
            case EAesReload::Always:     return QStringLiteral("Always");
            case EAesReload::Never:      return QStringLiteral("Never");
            case EAesReload::OncePerDay: return QStringLiteral("Once Per Day");
        }
        return QString();
    }

    inline QString description(EDiscordRpc value)
    {
        switch (value)
        {
            case EDiscordRpc::Always: return QStringLiteral("Always");
            case EDiscordRpc::Never:  return QStringLiteral("Never");
        }
        return QString();
    }

    inline QString description(ELoadingMode value)
    {
        switch (value)
        {
            case ELoadingMode::Multiple:       return QStringLiteral("Multiple");
            case ELoadingMode::All:            return QStringLiteral("All");
            case ELoadingMode::AllButNew:      return QStringLiteral("All (New)");
            case ELoadingMode::AllButModified: return QStringLiteral("All (Modified)");
            case ELoadingMode::AllButPatched:  return QStringLiteral("All (Except Patched Assets)");
        }
        return QString();
    }

    inline QString description(ECompressedAudio value)
    {
        switch (value)
        {
            case ECompressedAudio::PlayDecompressed: return QStringLiteral("Play the decompressed data");
            case ECompressedAudio::PlayCompressed:
                return QStringLiteral("Play the compressed data (might not always be a valid audio data)");
        }
        return QString();
    }

    inline QString description(EIconStyle value)
    {
        switch (value)
        {
            case EIconStyle::Default:      return QStringLiteral("Default");
            case EIconStyle::NoBackground: return QStringLiteral("No Background");
            case EIconStyle::NoText:       return QStringLiteral("No Text");
            case EIconStyle::Flat:         return QStringLiteral("Flat");
            case EIconStyle::Cataba:       return QStringLiteral("Cataba");
        }
        return QString();
    }

    // Equivalent to EAssetCategory.ToString() in C#. The categories combo binds the raw enum values and WPF
    // renders them through ToString(), so the port needs the member names.
    inline QString assetCategoryName(EAssetCategory value)
    {
        switch (value)
        {
            case EAssetCategory::All:                           return QStringLiteral("All");
            case EAssetCategory::Blueprints:                    return QStringLiteral("Blueprints");
            case EAssetCategory::BlueprintGeneratedClass:       return QStringLiteral("BlueprintGeneratedClass");
            case EAssetCategory::WidgetBlueprintGeneratedClass: return QStringLiteral("WidgetBlueprintGeneratedClass");
            case EAssetCategory::AnimBlueprintGeneratedClass:   return QStringLiteral("AnimBlueprintGeneratedClass");
            case EAssetCategory::RigVMBlueprintGeneratedClass:  return QStringLiteral("RigVMBlueprintGeneratedClass");
            case EAssetCategory::UserDefinedEnum:               return QStringLiteral("UserDefinedEnum");
            case EAssetCategory::UserDefinedStruct:             return QStringLiteral("UserDefinedStruct");
            case EAssetCategory::Blueprint:                     return QStringLiteral("Blueprint");
            case EAssetCategory::CookedMetaData:                return QStringLiteral("CookedMetaData");
            case EAssetCategory::Mesh:                          return QStringLiteral("Mesh");
            case EAssetCategory::StaticMesh:                    return QStringLiteral("StaticMesh");
            case EAssetCategory::SkeletalMesh:                  return QStringLiteral("SkeletalMesh");
            case EAssetCategory::CustomizableObject:            return QStringLiteral("CustomizableObject");
            case EAssetCategory::NaniteDisplacedMesh:           return QStringLiteral("NaniteDisplacedMesh");
            case EAssetCategory::Texture:                       return QStringLiteral("Texture");
            case EAssetCategory::Materials:                     return QStringLiteral("Materials");
            case EAssetCategory::Material:                      return QStringLiteral("Material");
            case EAssetCategory::MaterialEditorData:            return QStringLiteral("MaterialEditorData");
            case EAssetCategory::MaterialFunction:              return QStringLiteral("MaterialFunction");
            case EAssetCategory::MaterialFunctionEditorData:    return QStringLiteral("MaterialFunctionEditorData");
            case EAssetCategory::MaterialParameterCollection:   return QStringLiteral("MaterialParameterCollection");
            case EAssetCategory::Animation:                     return QStringLiteral("Animation");
            case EAssetCategory::Skeleton:                      return QStringLiteral("Skeleton");
            case EAssetCategory::Rig:                           return QStringLiteral("Rig");
            case EAssetCategory::Level:                         return QStringLiteral("Level");
            case EAssetCategory::World:                         return QStringLiteral("World");
            case EAssetCategory::BuildData:                     return QStringLiteral("BuildData");
            case EAssetCategory::LevelSequence:                 return QStringLiteral("LevelSequence");
            case EAssetCategory::Foliage:                       return QStringLiteral("Foliage");
            case EAssetCategory::Data:                          return QStringLiteral("Data");
            case EAssetCategory::ItemDefinitionBase:            return QStringLiteral("ItemDefinitionBase");
            case EAssetCategory::CurveBase:                     return QStringLiteral("CurveBase");
            case EAssetCategory::PhysicsAsset:                  return QStringLiteral("PhysicsAsset");
            case EAssetCategory::ObjectRedirector:              return QStringLiteral("ObjectRedirector");
            case EAssetCategory::PhysicalMaterial:              return QStringLiteral("PhysicalMaterial");
            case EAssetCategory::ByteCode:                      return QStringLiteral("ByteCode");
            case EAssetCategory::Media:                         return QStringLiteral("Media");
            case EAssetCategory::Audio:                         return QStringLiteral("Audio");
            case EAssetCategory::Video:                         return QStringLiteral("Video");
            case EAssetCategory::Font:                          return QStringLiteral("Font");
            case EAssetCategory::SoundBank:                     return QStringLiteral("SoundBank");
            case EAssetCategory::AudioEvent:                    return QStringLiteral("AudioEvent");
            case EAssetCategory::Particle:                      return QStringLiteral("Particle");
            case EAssetCategory::GameSpecific:                  return QStringLiteral("GameSpecific");
            case EAssetCategory::Borderlands:                   return QStringLiteral("Borderlands");
            case EAssetCategory::Aion2:                         return QStringLiteral("Aion2");
            case EAssetCategory::RocoKingdomWorld:              return QStringLiteral("RocoKingdomWorld");
            case EAssetCategory::DeltaForce:                    return QStringLiteral("DeltaForce");
            case EAssetCategory::LegoBatman:                    return QStringLiteral("LegoBatman");
        }
        return QString();
    }

    // Equivalent to EBuildKind.ToString() in C# — ApplicationViewModel.TitleExtra interpolates it.
    inline QString buildKindName(EBuildKind kind)
    {
        switch (kind)
        {
            case EBuildKind::Debug:   return QStringLiteral("Debug");
            case EBuildKind::Release: return QStringLiteral("Release");
            case EBuildKind::Unknown: return QStringLiteral("Unknown");
        }
        return QString();
    }

    // Equivalent to EStatusKind.ToString() in C# — returns the member name.
    inline QString statusKindName(EStatusKind kind)
    {
        switch (kind)
        {
            case EStatusKind::Ready:       return QStringLiteral("Ready");
            case EStatusKind::Configuring: return QStringLiteral("Configuring");
            case EStatusKind::Loading:     return QStringLiteral("Loading");
            case EStatusKind::Stopping:    return QStringLiteral("Stopping");
            case EStatusKind::Stopped:     return QStringLiteral("Stopped");
            case EStatusKind::Failed:      return QStringLiteral("Failed");
            case EStatusKind::Completed:   return QStringLiteral("Completed");
        }
        return QString();
    }
}
