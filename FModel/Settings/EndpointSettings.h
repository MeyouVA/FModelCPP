#pragma once
// Ported from FModel/Settings/EndpointSettings.cs.
//
// TryValidate is NOT ported: it drives a live request through DynamicApiEndpoint, and neither the API-endpoint
// view-models nor the HTTP layer exist yet. setIsValid() — which is what TryValidate ultimately assigns, and
// what the settings file persists — is here, so the type is complete for storage purposes.

#include "../Framework/ViewModel.h"

#include <QList>
#include <QString>

class QJsonObject;

namespace FModel::Settings
{
    class EndpointSettings : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        // Per-game endpoint defaults. Index 0 is EEndpointType::Aes, index 1 is EEndpointType::Mapping — the
        // C# code indexes this array with the enum directly, so the order is part of the contract.
        // Returns owned objects; the caller takes ownership.
        static QList<EndpointSettings*> Default(const QString& gameName);

        explicit EndpointSettings(QObject* parent = nullptr) : ViewModel(parent) {}
        EndpointSettings(QString url, QString path, QObject* parent = nullptr);

        const QString& url() const { return _url; }
        void setUrl(const QString& value) { setProperty(_url, value, QStringLiteral("Url")); }

        const QString& path() const { return _path; }
        void setPath(const QString& value) { setProperty(_path, value, QStringLiteral("Path")); }

        bool overwrite() const { return _overwrite; }
        void setOverwrite(bool value) { setProperty(_overwrite, value, QStringLiteral("Overwrite")); }

        const QString& filePath() const { return _filePath; }
        void setFilePath(const QString& value) { setProperty(_filePath, value, QStringLiteral("FilePath")); }

        bool isValid() const { return _isValid; }
        void setIsValid(bool value);

        // [JsonIgnore] in C# — derived, never persisted.
        QString label() const;

        QJsonObject toJson() const;
        void readJson(const QJsonObject& json);
        static EndpointSettings* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    private:
        QString _url;
        QString _path;
        bool _overwrite = false;
        QString _filePath;
        bool _isValid = false;
    };
}
