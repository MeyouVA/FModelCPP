#include "DirectorySettings.h"

#include <QJsonArray>
#include <QJsonObject>

#include "CustomDirectory.h"
#include "EndpointSettings.h"
#include "UserSettings.h"
#include "VersioningSettings.h"
#include "../Extensions/JsonExtensions.h"

namespace FModel::Settings
{
    DirectorySettings::DirectorySettings(QObject* parent)
        : ViewModel(parent), _versioning(new VersioningSettings(this))
    {
    }

    DirectorySettings* DirectorySettings::Default(const QString& gameName, const QString& gameDir, bool manual,
                                                  EGame ue, const QString& aes, QObject* parent)
    {
        DirectorySettings* old = UserSettings::Default()->perDirectory().value(gameDir, nullptr);

        auto* settings = new DirectorySettings(parent);
        settings->setGameName(gameName);
        settings->setGameDirectory(gameDir);
        settings->setIsManual(manual);
        settings->setUeVersion(old ? old->ueVersion() : ue);
        settings->setTexturePlatform(old ? old->texturePlatform() : ETexturePlatform::DesktopMobile);

        // C# reuses the *same* sub-objects from `old` (old?.Versioning ?? new ...). Here they are copied out of
        // `old` instead, for the ownership reason spelled out on clone(): a settings object owns its subtree.
        if (old)
        {
            settings->setVersioning(old->versioning()->clone());

            QList<EndpointSettings*> endpoints;
            endpoints.reserve(old->endpoints().size());
            for (const EndpointSettings* endpoint : old->endpoints())
                endpoints.append(EndpointSettings::fromJson(endpoint->toJson()));
            settings->setEndpoints(endpoints);

            QList<CustomDirectory*> directories;
            directories.reserve(old->directories().size());
            for (const CustomDirectory* directory : old->directories())
                directories.append(new CustomDirectory(directory->header(), directory->directoryPath()));
            settings->setDirectories(directories);

            settings->setAesKeys(old->aesKeys());
            settings->setLastAesReload(old->lastAesReload());
            settings->setCriwareDecryptionKey(old->criwareDecryptionKey());
            settings->setUnluacOpCodeMap(old->unluacOpCodeMap());
        }
        else
        {
            settings->setEndpoints(EndpointSettings::Default(gameName));
            settings->setDirectories(CustomDirectory::Default(gameName));

            AesResponse keys;
            keys.MainKey = aes;
            settings->setAesKeys(keys);

            // DateTime.Today.AddDays(-1) — midnight yesterday, so the first AES reload check always fires.
            settings->setLastAesReload(QDateTime(QDate::currentDate().addDays(-1), QTime(0, 0)));
            settings->setCriwareDecryptionKey(0);
            settings->setUnluacOpCodeMap(QString());
        }

        return settings;
    }

    void DirectorySettings::setVersioning(VersioningSettings* value)
    {
        if (_versioning == value)
            return;

        delete _versioning;
        _versioning = value;
        if (_versioning)
            _versioning->setParent(this);
        raisePropertyChanged(QStringLiteral("Versioning"));
    }

    void DirectorySettings::clearEndpoints()
    {
        qDeleteAll(_endpoints);
        _endpoints.clear();
    }

    void DirectorySettings::clearDirectories()
    {
        qDeleteAll(_directories);
        _directories.clear();
    }

    void DirectorySettings::setEndpoints(const QList<EndpointSettings*>& value)
    {
        if (_endpoints == value)
            return;

        clearEndpoints();
        _endpoints = value;
        for (EndpointSettings* endpoint : _endpoints)
            endpoint->setParent(this);
        raisePropertyChanged(QStringLiteral("Endpoints"));
    }

    void DirectorySettings::setDirectories(const QList<CustomDirectory*>& value)
    {
        if (_directories == value)
            return;

        clearDirectories();
        _directories = value;
        for (CustomDirectory* directory : _directories)
            directory->setParent(this);
        raisePropertyChanged(QStringLiteral("Directories"));
    }

    void DirectorySettings::setAesKeys(const AesResponse& value)
    {
        // AesResponse is a DTO without operator==; assign and notify unconditionally, which is what C# does
        // anyway (SetProperty compares by reference there, and every assignment is a fresh instance).
        _aesKeys = value;
        raisePropertyChanged(QStringLiteral("AesKeys"));
    }

    DirectorySettings* DirectorySettings::clone(QObject* parent) const
    {
        auto* copy = new DirectorySettings(parent);
        copy->readJson(toJson());
        copy->_aesKeys.Version = _aesKeys.Version; // [JsonIgnore], so it does not survive the round-trip above
        return copy;
    }

    QJsonObject DirectorySettings::toJson() const
    {
        using namespace FModel::Extensions;

        QJsonObject json;
        json[QStringLiteral("GameName")] = _gameName;
        json[QStringLiteral("GameDirectory")] = _gameDirectory;
        json[QStringLiteral("IsManual")] = _isManual;
        json[QStringLiteral("UeVersion")] = static_cast<qint64>(_ueVersion);
        json[QStringLiteral("TexturePlatform")] = enumToJson(_texturePlatform);
        json[QStringLiteral("Versioning")] = _versioning->toJson();

        QJsonArray endpoints;
        for (const EndpointSettings* endpoint : _endpoints)
            endpoints.append(endpoint->toJson());
        json[QStringLiteral("Endpoints")] = endpoints;

        QJsonArray directories;
        for (const CustomDirectory* directory : _directories)
            directories.append(directory->toJson());
        json[QStringLiteral("Directories")] = directories;

        json[QStringLiteral("AesKeys")] = _aesKeys.toJson();
        // Qualified: an unqualified toJson() here would find this class's own member and stop looking.
        json[QStringLiteral("LastAesReload")] = FModel::Extensions::toJson(_lastAesReload);
        json[QStringLiteral("CriwareDecryptionKey")] = static_cast<qint64>(_criwareDecryptionKey);
        json[QStringLiteral("UnluacOpCodeMap")] = _unluacOpCodeMap.isNull()
            ? QJsonValue(QJsonValue::Null)
            : QJsonValue(_unluacOpCodeMap);

        return json;
    }

    void DirectorySettings::readJson(const QJsonObject& json)
    {
        using namespace FModel::Extensions;

        setGameName(stringFromJson(json, QStringLiteral("GameName"), _gameName));
        setGameDirectory(stringFromJson(json, QStringLiteral("GameDirectory"), _gameDirectory));
        setIsManual(boolFromJson(json, QStringLiteral("IsManual"), _isManual));
        setUeVersion(static_cast<EGame>(int64FromJson(json, QStringLiteral("UeVersion"), static_cast<qint64>(_ueVersion))));
        setTexturePlatform(enumFromJson(json, QStringLiteral("TexturePlatform"), _texturePlatform));

        if (json.contains(QStringLiteral("Versioning")))
            setVersioning(VersioningSettings::fromJson(json[QStringLiteral("Versioning")].toObject()));

        if (json.contains(QStringLiteral("Endpoints")))
        {
            QList<EndpointSettings*> endpoints;
            const QJsonArray array = json[QStringLiteral("Endpoints")].toArray();
            endpoints.reserve(array.size());
            for (const QJsonValue& value : array)
                endpoints.append(EndpointSettings::fromJson(value.toObject()));
            setEndpoints(endpoints);
        }

        if (json.contains(QStringLiteral("Directories")))
        {
            QList<CustomDirectory*> directories;
            const QJsonArray array = json[QStringLiteral("Directories")].toArray();
            directories.reserve(array.size());
            for (const QJsonValue& value : array)
                directories.append(CustomDirectory::fromJson(value.toObject()));
            setDirectories(directories);
        }

        if (json.contains(QStringLiteral("AesKeys")))
            setAesKeys(AesResponse::fromJson(json[QStringLiteral("AesKeys")].toObject()));

        setLastAesReload(dateTimeFromJson(json[QStringLiteral("LastAesReload")], _lastAesReload));
        setCriwareDecryptionKey(static_cast<quint64>(
            int64FromJson(json, QStringLiteral("CriwareDecryptionKey"), static_cast<qint64>(_criwareDecryptionKey))));

        // `null` is a legitimate stored value here (older files wrote it), and it must not be read as "".
        const QJsonValue opCodeMap = json.value(QStringLiteral("UnluacOpCodeMap"));
        if (opCodeMap.isString())
            setUnluacOpCodeMap(opCodeMap.toString());
        else if (opCodeMap.isNull())
            setUnluacOpCodeMap(QString());
    }

    DirectorySettings* DirectorySettings::fromJson(const QJsonObject& json, QObject* parent)
    {
        auto* settings = new DirectorySettings(parent);
        settings->readJson(json);
        return settings;
    }
}
