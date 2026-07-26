#pragma once
// Ported from FModel/ViewModels/GameSelectorViewModel.cs — the directory selector's view-model: the list of
// game directories to choose from, the UE version list, and adding/removing a directory by hand.
//
// DetectedGame is the nested type UserSettings persists under "ManualGames" — a member the C# source itself
// marks "TO DELETEEEEEEEEEEEEE", but which real settings files still contain, so the port has to read and
// write it to round-trip them.
//
// Deliberate differences from C#:
//   * EnumerateDetectedGames' LAUNCHER SCANNERS ARE NOT PORTED. C# probes every drive for Epic's
//     LauncherInstalled.dat, Riot's RiotClientInstalls.json, Steam's libraryfolders.vdf + .acf app
//     manifests, and two Windows registry keys (Rockstar, Level Infinite) to auto-detect ~20 games. That is
//     a pile of platform-specific format parsing that has nothing to do with the port's spine; the list is
//     instead built from what settings already knows plus the two live-service entries, and a user adds a
//     directory by browsing to it. detectedLaunchers() reports that the scan was skipped.
//   * `AddUndetectedDir(gameDirectory)` splits on '\' as C# does, so a path with forward slashes keeps its
//     whole string as the name — upstream behaves the same way.
//   * TryGetUeVersionFromExe reads the Win32 version resource directly (C# uses FileVersionInfo, which is
//     the same data). On a non-Windows build it always fails, which lands on the same GAME_UE4_LATEST
//     fallback C# uses when the read throws.
//   * DetectedDirectories owns its entries (QObject children); C# leaves them to the GC.

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>

#include "UE4/Objects/Core/Serialization/FCustomVersion.h"
#include "UE4/Versions/EGame.h"

#include "ApiEndpoints/Models/AesResponse.h"
#include "../Framework/ViewModel.h"

class QJsonObject;

namespace FModel::Settings { class CustomDirectory; class DirectorySettings; }

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

    class GameSelectorViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        explicit GameSelectorViewModel(const QString& gameDirectory, QObject* parent = nullptr);

        Settings::DirectorySettings* selectedDirectory() const { return _selectedDirectory; }
        void setSelectedDirectory(Settings::DirectorySettings* value);

        const QList<Settings::DirectorySettings*>& detectedDirectories() const { return _detectedDirectories; }
        const QList<CUE4Parse::UE4::Versions::EGame>& ueGames() const { return _ueGames; }

        // C#: AddUndetectedDir(gameDirectory) => AddUndetectedDir(gameDirectory.SubstringAfterLast('\\'), ...)
        void addUndetectedDir(const QString& gameDirectory);
        void addUndetectedDir(const QString& gameName, const QString& gameDirectory);

        void deleteSelectedGame();

        // C#'s private TryDetectUeVersion: walks down to a Paks folder, then up to the project's
        // Binaries\Win64 and finally the engine's CrashReportClient, reading the UE version off an exe.
        // Returns whether a version was found; `ueVersion` is GAME_UE4_LATEST when it was not, as upstream.
        static bool tryDetectUeVersion(const QString& gameDirectory,
                                       CUE4Parse::UE4::Versions::EGame& ueVersion,
                                       QString& newGameDirectory);
        static bool tryGetUeVersionFromExe(const QString& exePath, CUE4Parse::UE4::Versions::EGame& ueVersion);

        // C#'s private EnumerateUeGames(). SettingsViewModel declares a character-for-character IDENTICAL
        // copy of this method; rather than duplicate it, that one delegates here. (This is the lower of the
        // two in the build's layering, which is why the implementation lives on this side.)
        static QList<CUE4Parse::UE4::Versions::EGame> enumerateUeGames();

    private:
        QList<Settings::DirectorySettings*> enumerateDetectedGames();

        Settings::DirectorySettings* _selectedDirectory = nullptr;
        QList<Settings::DirectorySettings*> _detectedDirectories;
        QList<CUE4Parse::UE4::Versions::EGame> _ueGames;
    };
}
