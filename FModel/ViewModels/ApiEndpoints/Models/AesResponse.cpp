#include "AesResponse.h"

#include <QJsonArray>
#include <QJsonObject>

namespace FModel::ViewModels::ApiEndpoints::Models
{
    QJsonObject DynamicKey::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("name")] = Name;
        json[QStringLiteral("guid")] = Guid;
        json[QStringLiteral("key")] = Key;
        return json;
    }

    DynamicKey DynamicKey::fromJson(const QJsonObject& json)
    {
        DynamicKey key;
        key.Name = json[QStringLiteral("name")].toString();
        key.Guid = json[QStringLiteral("guid")].toString();
        key.Key = json[QStringLiteral("key")].toString();
        return key;
    }

    QJsonObject AesResponse::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("mainKey")] = MainKey;

        // C# leaves DynamicKeys null when the endpoint has none (DirectorySettings.Default writes exactly that),
        // and Newtonsoft emits `null` for it. Reproduced so existing files round-trip byte-for-byte.
        if (DynamicKeys.isEmpty())
        {
            json[QStringLiteral("dynamicKeys")] = QJsonValue(QJsonValue::Null);
        }
        else
        {
            QJsonArray keys;
            for (const DynamicKey& key : DynamicKeys)
                keys.append(key.toJson());
            json[QStringLiteral("dynamicKeys")] = keys;
        }

        return json;
    }

    AesResponse AesResponse::fromJson(const QJsonObject& json)
    {
        AesResponse response;
        response.MainKey = json[QStringLiteral("mainKey")].toString();

        const QJsonArray keys = json[QStringLiteral("dynamicKeys")].toArray();
        response.DynamicKeys.reserve(keys.size());
        for (const QJsonValue& key : keys)
            response.DynamicKeys.append(DynamicKey::fromJson(key.toObject()));

        return response;
    }
}
