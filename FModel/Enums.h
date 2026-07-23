#pragma once
// Ported from FModel/Enums.cs.
//
// C#'s [Description] attribute (read by the settings UI through EnumExtensions.GetDescription) has no direct
// C++ equivalent. It is reproduced here as an overload set: one `description(E)` function per enum, resolved
// by argument type. The same convention is used for the CUE4Parse-side enums, where it is spelled
// `Description(E)` and returns const char* so that library stays free of Qt.
//
// Still deferred: EAssetCategory, which depends on AssetCategoryExtensions.CategoryBase.
//
// statusKindName() reproduces C#'s enum.ToString() (the member name), which FStatus uses to build its label.

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
