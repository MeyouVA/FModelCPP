#include "EndpointSettings.h"

#include <QJsonObject>

#include "../Extensions/JsonExtensions.h"

namespace FModel::Settings
{
    QList<EndpointSettings*> EndpointSettings::Default(const QString& gameName)
    {
        if (gameName == QLatin1String("Fortnite") || gameName == QLatin1String("Fortnite [LIVE]"))
        {
            return {
                new EndpointSettings(QStringLiteral("https://uedb.dev/svc/api/v1/fortnite/aes"), QStringLiteral("$.['mainKey','dynamicKeys']")),
                new EndpointSettings(QStringLiteral("https://uedb.dev/svc/api/v1/fortnite/mappings"), QStringLiteral("$.mappings.ZStandard"))
            };
        }

        if (gameName == QLatin1String("VALORANT") || gameName == QLatin1String("VALORANT [LIVE]"))
        {
            return {
                new EndpointSettings(QStringLiteral("https://uedb.dev/svc/api/v1/valorant/aes"), QStringLiteral("$.['mainKey','dynamicKeys']")),
                new EndpointSettings(QStringLiteral("https://uedb.dev/svc/api/v1/valorant/mappings"), QStringLiteral("$.mappings.ZStandard"))
            };
        }

        return { new EndpointSettings(), new EndpointSettings() };
    }

    EndpointSettings::EndpointSettings(QString url, QString path, QObject* parent)
        : ViewModel(parent), _url(std::move(url)), _path(std::move(path))
    {
        // "be careful with this" — the C# comment. The two-argument constructor pre-trusts its own defaults;
        // the default-constructed (empty) endpoint stays invalid.
        _isValid = !_url.isEmpty() && !_path.isEmpty();
    }

    void EndpointSettings::setIsValid(bool value)
    {
        // C# calls SetProperty and then raises Label unconditionally, so Label refreshes even on a no-op set.
        setProperty(_isValid, value, QStringLiteral("IsValid"));
        raisePropertyChanged(QStringLiteral("Label"));
    }

    QString EndpointSettings::label() const
    {
        return _isValid
            ? QStringLiteral("Your endpoint configuration is valid! Please, avoid any unnecessary modifications!")
            : QStringLiteral("Your endpoint configuration DOES NOT seem to be valid yet! Please, test it out!");
    }

    QJsonObject EndpointSettings::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("Url")] = _url;
        json[QStringLiteral("Path")] = _path;
        json[QStringLiteral("Overwrite")] = _overwrite;
        json[QStringLiteral("FilePath")] = _filePath;
        json[QStringLiteral("IsValid")] = _isValid;
        return json;
    }

    void EndpointSettings::readJson(const QJsonObject& json)
    {
        using namespace FModel::Extensions;
        setUrl(stringFromJson(json, QStringLiteral("Url"), _url));
        setPath(stringFromJson(json, QStringLiteral("Path"), _path));
        setOverwrite(boolFromJson(json, QStringLiteral("Overwrite"), _overwrite));
        setFilePath(stringFromJson(json, QStringLiteral("FilePath"), _filePath));
        setIsValid(boolFromJson(json, QStringLiteral("IsValid"), _isValid));
    }

    EndpointSettings* EndpointSettings::fromJson(const QJsonObject& json, QObject* parent)
    {
        auto* settings = new EndpointSettings(parent);
        settings->readJson(json);
        return settings;
    }
}
