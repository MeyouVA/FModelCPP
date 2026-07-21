#pragma once
// Ported from FModel/Enums.cs.
//
// The plain enums (no attributes, no cross-type dependencies) are ported now as scoped enums.
// Deferred until the layers that need them arrive:
//   - EAesReload / EDiscordRpc / ELoadingMode / ECompressedAudio / EIconStyle carry [Description]
//     attributes used by the Settings UI — they need a description mechanism first.
//   - EAssetCategory depends on AssetCategoryExtensions.CategoryBase.
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
