#include "CustomDirectory.h"

#include <QJsonObject>

namespace FModel::Settings
{
    QList<CustomDirectory*> CustomDirectory::Default(const QString& gameName)
    {
        if (gameName == QLatin1String("Fortnite") || gameName == QLatin1String("Fortnite [LIVE]"))
        {
            return {
                new CustomDirectory(QStringLiteral("Cosmetics"), QStringLiteral("FortniteGame/Plugins/GameFeatures/BRCosmetics/Content/Athena/Items/Cosmetics/")),
                new CustomDirectory(QStringLiteral("Emotes [AUDIO]"), QStringLiteral("FortniteGame/Plugins/GameFeatures/BRCosmetics/Content/Athena/Sounds/Emotes/")),
                new CustomDirectory(QStringLiteral("Music Packs [AUDIO]"), QStringLiteral("FortniteGame/Plugins/GameFeatures/BRCosmetics/Content/Athena/Sounds/MusicPacks/")),
                new CustomDirectory(QStringLiteral("Weapons"), QStringLiteral("FortniteGame/Content/Athena/Items/Weapons/")),
                new CustomDirectory(QStringLiteral("Strings"), QStringLiteral("FortniteGame/Content/Localization/"))
            };
        }

        if (gameName == QLatin1String("VALORANT") || gameName == QLatin1String("VALORANT [LIVE]"))
        {
            return {
                new CustomDirectory(QStringLiteral("Audio"), QStringLiteral("ShooterGame/Content/WwiseAudio/Media/")),
                new CustomDirectory(QStringLiteral("Characters"), QStringLiteral("ShooterGame/Content/Characters/")),
                new CustomDirectory(QStringLiteral("Gun Buddies"), QStringLiteral("ShooterGame/Content/Equippables/Buddies/")),
                new CustomDirectory(QStringLiteral("Cards and Sprays"), QStringLiteral("ShooterGame/Content/Personalization/")),
                new CustomDirectory(QStringLiteral("Shop Backgrounds"), QStringLiteral("ShooterGame/Content/UI/OutOfGame/MainMenu/Store/Shared/Textures/")),
                new CustomDirectory(QStringLiteral("Weapon Renders"), QStringLiteral("ShooterGame/Content/UI/Screens/OutOfGame/MainMenu/Collection/Assets/Large/"))
            };
        }

        if (gameName == QLatin1String("Dead by Daylight"))
        {
            return {
                new CustomDirectory(QStringLiteral("Characters V1"), QStringLiteral("DeadByDaylight/Plugins/DBDCharacters/")),
                new CustomDirectory(QStringLiteral("Characters V2"), QStringLiteral("DeadByDaylight/Plugins/Runtime/Bhvr/DBDCharacters/")),
                new CustomDirectory(QStringLiteral("Characters (Deprecated)"), QStringLiteral("DeadbyDaylight/Content/Characters/")),
                new CustomDirectory(QStringLiteral("Meshes"), QStringLiteral("DeadByDaylight/Content/Meshes/")),
                new CustomDirectory(QStringLiteral("Textures"), QStringLiteral("DeadByDaylight/Content/Textures/")),
                new CustomDirectory(QStringLiteral("Icons"), QStringLiteral("DeadByDaylight/Content/UI/UMGAssets/Icons/")),
                new CustomDirectory(QStringLiteral("Blueprints"), QStringLiteral("DeadByDaylight/Content/Blueprints/")),
                new CustomDirectory(QStringLiteral("Audio Events"), QStringLiteral("DeadByDaylight/Content/Audio/Events/")),
                new CustomDirectory(QStringLiteral("Audio"), QStringLiteral("DeadByDaylight/Content/WwiseAudio/Cooked/")),
                new CustomDirectory(QStringLiteral("Data Tables"), QStringLiteral("DeadByDaylight/Content/Data/")),
                new CustomDirectory(QStringLiteral("Localization"), QStringLiteral("DeadByDaylight/Content/Localization/"))
            };
        }

        return {};
    }

    QJsonObject CustomDirectory::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("Header")] = _header;
        json[QStringLiteral("DirectoryPath")] = _directoryPath;
        return json;
    }

    CustomDirectory* CustomDirectory::fromJson(const QJsonObject& json, QObject* parent)
    {
        return new CustomDirectory(json[QStringLiteral("Header")].toString(),
                                   json[QStringLiteral("DirectoryPath")].toString(),
                                   parent);
    }
}
