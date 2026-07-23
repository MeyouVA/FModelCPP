#include "EpicResponse.h"

#include <QJsonObject>

#include "../../../Extensions/JsonExtensions.h"

namespace FModel::ViewModels::ApiEndpoints::Models
{
    QJsonObject AuthResponse::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("access_token")] = AccessToken;
        json[QStringLiteral("expires_at")] = Extensions::toJson(ExpiresAt);
        return json;
    }

    AuthResponse AuthResponse::fromJson(const QJsonObject& json)
    {
        AuthResponse response;
        response.AccessToken = json[QStringLiteral("access_token")].toString();
        response.ExpiresAt = Extensions::dateTimeFromJson(json[QStringLiteral("expires_at")]);
        return response;
    }
}
