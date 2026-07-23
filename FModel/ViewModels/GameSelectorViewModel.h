#pragma once
// Ported from FModel/ViewModels/GameSelectorViewModel.cs — the nested DetectedGame type only.
//
// The view-model itself (game auto-detection, the UE-version list, AddUndetectedDir, ...) is not ported yet.
// DetectedGame comes first because UserSettings persists a map of them under "ManualGames" — a member the C#
// source itself marks "TO DELETEEEEEEEEEEEEE", but which real settings files still contain, so the port has to
// read and write it to round-trip them.

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>

#include "UE4/Objects/Core/Serialization/FCustomVersion.h"
#include "UE4/Versions/EGame.h"

#include "ApiEndpoints/Models/AesResponse.h"

class QJsonObject;

namespace FModel::Settings { class CustomDirectory; }

namespace FModel::ViewModels
{
    // Plain data bag in C# (auto-properties, no ViewModel base), so a plain struct here.
    struct DetectedGame
    {
        QString GameName;
        QString GameDirectory;
        CUE4Parse::UE4::Versions::EGame OverridedGame = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        bool IsManual = false;

        // the followings are only used when game is manually added
        ApiEndpoints::Models::AesResponse AesKeys;
        QList<CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersion> OverridedCustomVersions;
        QHash<QString, bool> OverridedOptions;
        QHash<QString, QPair<QString, QString>> OverridedMapStructTypes;
        QList<QPair<QString, QString>> CustomDirectories; // (Header, DirectoryPath)

        QJsonObject toJson() const;
        static DetectedGame fromJson(const QJsonObject& json);
    };
}
