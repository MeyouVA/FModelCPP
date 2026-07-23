#pragma once
// Ported from FModel/ViewModels/ApiEndpoints/Models/AesResponse.cs.
//
// These are plain DTOs (not ViewModels), so they stay plain C++ structs. Unlike the rest of the settings tree
// they serialise with *camelCase* names — the C# source pins them with [JsonProperty] because they are also
// the wire format of the AES endpoint. AppSettings.json embeds them verbatim, so the casing is load-bearing.
//
// `Version` is [JsonIgnore] in C#: it is populated from the API response but never persisted.

#include <QString>
#include <QList>

class QJsonObject;

namespace FModel::ViewModels::ApiEndpoints::Models
{
    struct DynamicKey
    {
        QString Name;  // "name"
        QString Guid;  // "guid"
        QString Key;   // "key"

        bool isValid() const { return Guid.length() == 32 && Key.length() == 66; }

        QJsonObject toJson() const;
        static DynamicKey fromJson(const QJsonObject& json);
    };

    struct AesResponse
    {
        QString Version;              // [JsonIgnore]
        QString MainKey;              // "mainKey"
        QList<DynamicKey> DynamicKeys; // "dynamicKeys"

        bool hasDynamicKeys() const { return !DynamicKeys.isEmpty(); }
        bool isValid() const { return MainKey.length() == 66 || hasDynamicKeys(); }

        QJsonObject toJson() const;
        static AesResponse fromJson(const QJsonObject& json);
    };
}
