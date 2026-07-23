#pragma once
// Ported from FModel/ViewModels/ApiEndpoints/Models/EpicResponse.cs.
//
// Only AuthResponse is ported: it is the one member of this file that UserSettings persists
// (LastAuthResponse). Its snake_case JSON names come straight from the Epic OAuth response.

#include <QString>
#include <QDateTime>

class QJsonObject;

namespace FModel::ViewModels::ApiEndpoints::Models
{
    struct AuthResponse
    {
        QString AccessToken;   // "access_token"
        QDateTime ExpiresAt;   // "expires_at"

        QJsonObject toJson() const;
        static AuthResponse fromJson(const QJsonObject& json);
    };
}
