#include "GameSelectorViewModel.h"

#include <QJsonArray>
#include <QJsonObject>

#include "UE4/Objects/Core/Misc/FGuid.h"

namespace FModel::ViewModels
{
    using CUE4Parse::UE4::Objects::Core::Misc::EGuidFormats;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersion;

    QJsonObject DetectedGame::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("GameName")] = GameName;
        json[QStringLiteral("GameDirectory")] = GameDirectory;
        json[QStringLiteral("OverridedGame")] = static_cast<qint64>(OverridedGame);
        json[QStringLiteral("IsManual")] = IsManual;
        json[QStringLiteral("AesKeys")] = AesKeys.toJson();

        QJsonArray versions;
        for (const FCustomVersion& version : OverridedCustomVersions)
        {
            QJsonObject entry;
            entry[QStringLiteral("Key")] = QString::fromStdString(version.Key.ToString(EGuidFormats::UniqueObjectGuid));
            entry[QStringLiteral("Version")] = version.Version;
            versions.append(entry);
        }
        json[QStringLiteral("OverridedCustomVersions")] = versions;

        QJsonObject options;
        for (auto it = OverridedOptions.constBegin(); it != OverridedOptions.constEnd(); ++it)
            options[it.key()] = it.value();
        json[QStringLiteral("OverridedOptions")] = options;

        QJsonObject mapStructTypes;
        for (auto it = OverridedMapStructTypes.constBegin(); it != OverridedMapStructTypes.constEnd(); ++it)
        {
            QJsonObject pair;
            pair[QStringLiteral("Key")] = it.value().first;
            pair[QStringLiteral("Value")] = it.value().second;
            mapStructTypes[it.key()] = pair;
        }
        json[QStringLiteral("OverridedMapStructTypes")] = mapStructTypes;

        QJsonArray directories;
        for (const QPair<QString, QString>& directory : CustomDirectories)
        {
            QJsonObject entry;
            entry[QStringLiteral("Header")] = directory.first;
            entry[QStringLiteral("DirectoryPath")] = directory.second;
            directories.append(entry);
        }
        json[QStringLiteral("CustomDirectories")] = directories;

        return json;
    }

    DetectedGame DetectedGame::fromJson(const QJsonObject& json)
    {
        DetectedGame game;
        game.GameName = json[QStringLiteral("GameName")].toString();
        game.GameDirectory = json[QStringLiteral("GameDirectory")].toString();
        game.OverridedGame = static_cast<CUE4Parse::UE4::Versions::EGame>(
            json[QStringLiteral("OverridedGame")].toInteger(CUE4Parse::UE4::Versions::GAME_UE4_LATEST));
        game.IsManual = json[QStringLiteral("IsManual")].toBool();
        game.AesKeys = ApiEndpoints::Models::AesResponse::fromJson(json[QStringLiteral("AesKeys")].toObject());

        for (const QJsonValue& value : json[QStringLiteral("OverridedCustomVersions")].toArray())
        {
            const QJsonObject entry = value.toObject();
            const QString digits = entry[QStringLiteral("Key")].toString().remove(QLatin1Char('-'));
            const FGuid key = digits.size() >= 32 ? FGuid(digits.toStdString()) : FGuid();
            game.OverridedCustomVersions.append(FCustomVersion(key, entry[QStringLiteral("Version")].toInt()));
        }

        const QJsonObject options = json[QStringLiteral("OverridedOptions")].toObject();
        for (auto it = options.constBegin(); it != options.constEnd(); ++it)
            game.OverridedOptions.insert(it.key(), it.value().toBool());

        const QJsonObject mapStructTypes = json[QStringLiteral("OverridedMapStructTypes")].toObject();
        for (auto it = mapStructTypes.constBegin(); it != mapStructTypes.constEnd(); ++it)
        {
            const QJsonObject pair = it.value().toObject();
            game.OverridedMapStructTypes.insert(
                it.key(), qMakePair(pair[QStringLiteral("Key")].toString(), pair[QStringLiteral("Value")].toString()));
        }

        for (const QJsonValue& value : json[QStringLiteral("CustomDirectories")].toArray())
        {
            const QJsonObject entry = value.toObject();
            game.CustomDirectories.append(qMakePair(entry[QStringLiteral("Header")].toString(),
                                                    entry[QStringLiteral("DirectoryPath")].toString()));
        }

        return game;
    }
}
