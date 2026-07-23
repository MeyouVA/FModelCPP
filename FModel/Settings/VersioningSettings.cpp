#include "VersioningSettings.h"

#include <QJsonArray>
#include <QJsonObject>

#include "UE4/Objects/Core/Misc/FGuid.h"

namespace FModel::Settings
{
    using CUE4Parse::UE4::Objects::Core::Misc::EGuidFormats;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

    namespace
    {
        // FGuid carries [JsonConverter(typeof(FGuidConverter))], which writes the UniqueObjectGuid form
        // ("00000000-00000000-00000000-00000000") and reads it back by stripping the hyphens.
        QString guidToJson(const FGuid& guid)
        {
            return QString::fromStdString(guid.ToString(EGuidFormats::UniqueObjectGuid));
        }

        FGuid guidFromJson(const QString& text)
        {
            const QString digits = QString(text).remove(QLatin1Char('-'));
            if (digits.size() < 32)
                return FGuid();

            return FGuid(digits.toStdString());
        }
    }

    VersioningSettings* VersioningSettings::clone(QObject* parent) const
    {
        auto* copy = new VersioningSettings(parent);
        copy->_customVersions = _customVersions;
        copy->_options = _options;
        copy->_mapStructTypes = _mapStructTypes;
        return copy;
    }

    QJsonObject VersioningSettings::toJson() const
    {
        QJsonObject json;

        QJsonArray versions;
        for (const FCustomVersion& version : _customVersions)
        {
            QJsonObject entry;
            entry[QStringLiteral("Key")] = guidToJson(version.Key);
            entry[QStringLiteral("Version")] = version.Version;
            versions.append(entry);
        }
        json[QStringLiteral("CustomVersions")] = versions;

        QJsonObject options;
        for (auto it = _options.constBegin(); it != _options.constEnd(); ++it)
            options[it.key()] = it.value();
        json[QStringLiteral("Options")] = options;

        QJsonObject mapStructTypes;
        for (auto it = _mapStructTypes.constBegin(); it != _mapStructTypes.constEnd(); ++it)
        {
            QJsonObject pair;
            pair[QStringLiteral("Key")] = it.value().first;
            pair[QStringLiteral("Value")] = it.value().second;
            mapStructTypes[it.key()] = pair;
        }
        json[QStringLiteral("MapStructTypes")] = mapStructTypes;

        return json;
    }

    void VersioningSettings::readJson(const QJsonObject& json)
    {
        QList<FCustomVersion> versions;
        const QJsonArray versionArray = json[QStringLiteral("CustomVersions")].toArray();
        versions.reserve(versionArray.size());
        for (const QJsonValue& value : versionArray)
        {
            const QJsonObject entry = value.toObject();
            versions.append(FCustomVersion(guidFromJson(entry[QStringLiteral("Key")].toString()),
                                           entry[QStringLiteral("Version")].toInt()));
        }
        setCustomVersions(versions);

        QHash<QString, bool> options;
        const QJsonObject optionObject = json[QStringLiteral("Options")].toObject();
        for (auto it = optionObject.constBegin(); it != optionObject.constEnd(); ++it)
            options.insert(it.key(), it.value().toBool());
        setOptions(options);

        QHash<QString, MapStructType> mapStructTypes;
        const QJsonObject mapObject = json[QStringLiteral("MapStructTypes")].toObject();
        for (auto it = mapObject.constBegin(); it != mapObject.constEnd(); ++it)
        {
            const QJsonObject pair = it.value().toObject();
            mapStructTypes.insert(it.key(), MapStructType(pair[QStringLiteral("Key")].toString(),
                                                          pair[QStringLiteral("Value")].toString()));
        }
        setMapStructTypes(mapStructTypes);
    }

    VersioningSettings* VersioningSettings::fromJson(const QJsonObject& json, QObject* parent)
    {
        auto* settings = new VersioningSettings(parent);
        settings->readJson(json);
        return settings;
    }
}
