#pragma once
// Ported from FModel/Settings/VersioningSettings.cs.
//
// The three collections are the per-game overrides handed to CUE4Parse's VersionContainer: the custom-version
// list, the boolean option map, and the map-property struct-type hints (C#'s
// IDictionary<string, KeyValuePair<string, string>>, which becomes a QHash<QString, QPair<QString, QString>>).

#include "../Framework/ViewModel.h"

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>

#include "UE4/Objects/Core/Serialization/FCustomVersion.h"

class QJsonObject;

namespace FModel::Settings
{
    using FCustomVersion = CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersion;

    // C#'s KeyValuePair<string, string> serialises as {"Key": ..., "Value": ...}; QPair keeps that shape.
    using MapStructType = QPair<QString, QString>;

    class VersioningSettings : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit VersioningSettings(QObject* parent = nullptr) : ViewModel(parent) {}

        const QList<FCustomVersion>& customVersions() const { return _customVersions; }
        void setCustomVersions(const QList<FCustomVersion>& value)
        {
            setProperty(_customVersions, value, QStringLiteral("CustomVersions"));
        }

        const QHash<QString, bool>& options() const { return _options; }
        void setOptions(const QHash<QString, bool>& value) { setProperty(_options, value, QStringLiteral("Options")); }

        const QHash<QString, MapStructType>& mapStructTypes() const { return _mapStructTypes; }
        void setMapStructTypes(const QHash<QString, MapStructType>& value)
        {
            setProperty(_mapStructTypes, value, QStringLiteral("MapStructTypes"));
        }

        // Deep copy, for DirectorySettings::clone().
        VersioningSettings* clone(QObject* parent = nullptr) const;

        QJsonObject toJson() const;
        void readJson(const QJsonObject& json);
        static VersioningSettings* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    private:
        QList<FCustomVersion> _customVersions;
        QHash<QString, bool> _options;
        QHash<QString, MapStructType> _mapStructTypes;
    };
}
