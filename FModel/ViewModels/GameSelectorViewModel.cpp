#include "GameSelectorViewModel.h"

#include <algorithm>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "version.lib")
#endif

#include "UE4/Objects/Core/Misc/FGuid.h"

#include "SettingsViewModel.h"
#include "../Constants.h"
#include "../Settings/DirectorySettings.h"
#include "../Settings/UserSettings.h"

namespace FModel::ViewModels
{
    using CUE4Parse::UE4::Objects::Core::Misc::EGuidFormats;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersion;

    QJsonObject DetectedGame::toJson() const
    {
        QJsonObject json;
        json[QStringLiteral("GameName")] = GameName;
        json[QStringLiteral("GameDirectory")] = GameDirectory;
        json[QStringLiteral("OverridedGame")] = static_cast<qint64>(OverridedGame);
        json[QStringLiteral("IsManual")] = IsManual;
        json[QStringLiteral("AesKeys")] = AesKeys.toJson();

        QJsonArray versions;
        for (const FCustomVersion& version : OverridedCustomVersions)
        {
            QJsonObject entry;
            entry[QStringLiteral("Key")] = QString::fromStdString(version.Key.ToString(EGuidFormats::UniqueObjectGuid));
            entry[QStringLiteral("Version")] = version.Version;
            versions.append(entry);
        }
        json[QStringLiteral("OverridedCustomVersions")] = versions;

        QJsonObject options;
        for (auto it = OverridedOptions.constBegin(); it != OverridedOptions.constEnd(); ++it)
            options[it.key()] = it.value();
        json[QStringLiteral("OverridedOptions")] = options;

        QJsonObject mapStructTypes;
        for (auto it = OverridedMapStructTypes.constBegin(); it != OverridedMapStructTypes.constEnd(); ++it)
        {
            QJsonObject pair;
            pair[QStringLiteral("Key")] = it.value().first;
            pair[QStringLiteral("Value")] = it.value().second;
            mapStructTypes[it.key()] = pair;
        }
        json[QStringLiteral("OverridedMapStructTypes")] = mapStructTypes;

        QJsonArray directories;
        for (const QPair<QString, QString>& directory : CustomDirectories)
        {
            QJsonObject entry;
            entry[QStringLiteral("Header")] = directory.first;
            entry[QStringLiteral("DirectoryPath")] = directory.second;
            directories.append(entry);
        }
        json[QStringLiteral("CustomDirectories")] = directories;

        return json;
    }

    DetectedGame DetectedGame::fromJson(const QJsonObject& json)
    {
        DetectedGame game;
        game.GameName = json[QStringLiteral("GameName")].toString();
        game.GameDirectory = json[QStringLiteral("GameDirectory")].toString();
        game.OverridedGame = static_cast<CUE4Parse::UE4::Versions::EGame>(
            json[QStringLiteral("OverridedGame")].toInteger(CUE4Parse::UE4::Versions::GAME_UE4_LATEST));
        game.IsManual = json[QStringLiteral("IsManual")].toBool();
        game.AesKeys = ApiEndpoints::Models::AesResponse::fromJson(json[QStringLiteral("AesKeys")].toObject());

        for (const QJsonValue& value : json[QStringLiteral("OverridedCustomVersions")].toArray())
        {
            const QJsonObject entry = value.toObject();
            const QString digits = entry[QStringLiteral("Key")].toString().remove(QLatin1Char('-'));
            const FGuid key = digits.size() >= 32 ? FGuid(digits.toStdString()) : FGuid();
            game.OverridedCustomVersions.append(FCustomVersion(key, entry[QStringLiteral("Version")].toInt()));
        }

        const QJsonObject options = json[QStringLiteral("OverridedOptions")].toObject();
        for (auto it = options.constBegin(); it != options.constEnd(); ++it)
            game.OverridedOptions.insert(it.key(), it.value().toBool());

        const QJsonObject mapStructTypes = json[QStringLiteral("OverridedMapStructTypes")].toObject();
        for (auto it = mapStructTypes.constBegin(); it != mapStructTypes.constEnd(); ++it)
        {
            const QJsonObject pair = it.value().toObject();
            game.OverridedMapStructTypes.insert(
                it.key(), qMakePair(pair[QStringLiteral("Key")].toString(), pair[QStringLiteral("Value")].toString()));
        }

        for (const QJsonValue& value : json[QStringLiteral("CustomDirectories")].toArray())
        {
            const QJsonObject entry = value.toObject();
            game.CustomDirectories.append(qMakePair(entry[QStringLiteral("Header")].toString(),
                                                    entry[QStringLiteral("DirectoryPath")].toString()));
        }

        return game;
    }

    // ---------------------------------------------------------------- GameSelectorViewModel

    GameSelectorViewModel::GameSelectorViewModel(const QString& gameDirectory, QObject* parent)
        : ViewModel(parent)
    {
        _detectedDirectories = enumerateDetectedGames();

        // Every manually added directory settings already knows about.
        auto* settings = Settings::UserSettings::Default();
        for (Settings::DirectorySettings* dir : settings->perDirectory())
        {
            if (dir != nullptr && dir->isManual())
                _detectedDirectories.append(dir->clone(this));
        }

        Settings::DirectorySettings* detectedGame = nullptr;
        for (Settings::DirectorySettings* dir : _detectedDirectories)
        {
            if (dir->gameDirectory() == gameDirectory)
            {
                detectedGame = dir;
                break;
            }
        }

        if (detectedGame != nullptr)
            setSelectedDirectory(detectedGame);
        else if (!gameDirectory.isEmpty())
            addUndetectedDir(gameDirectory);
        else
            setSelectedDirectory(_detectedDirectories.isEmpty() ? nullptr : _detectedDirectories.first());

        // Same list, and the same two-pass ordering, as the Settings dialog's UE-version combo.
        _ueGames = enumerateUeGames();
    }

    QList<CUE4Parse::UE4::Versions::EGame> GameSelectorViewModel::enumerateUeGames()
    {
        using namespace CUE4Parse::UE4::Versions;

        // C#:
        //     Enum.GetValues<EGame>()
        //         .GroupBy(value => (int) value)
        //         .Select(group => group.First())
        //         .OrderBy(value => ((int) value & 0xFF) == 0);
        //
        // EGameValues() is already the deduplicated list (see its note in EGame.h), so only the ordering is
        // left. OrderBy on a bool puts false first: the games, whose low byte is their offset from the base
        // version, come before the base versions themselves. OrderBy is a STABLE sort, so within each group
        // the members keep their ascending-by-value order — hence the two passes rather than a comparator.
        size_t count = 0;
        const EGame* values = EGameValues(count);

        QList<EGame> games;
        games.reserve(static_cast<qsizetype>(count));
        for (size_t i = 0; i < count; ++i)
        {
            if ((static_cast<int>(values[i]) & 0xFF) != 0)
                games.append(values[i]);
        }
        for (size_t i = 0; i < count; ++i)
        {
            if ((static_cast<int>(values[i]) & 0xFF) == 0)
                games.append(values[i]);
        }

        return games;
    }

    void GameSelectorViewModel::setSelectedDirectory(Settings::DirectorySettings* value)
    {
        if (_selectedDirectory == value)
            return;
        _selectedDirectory = value;
        raisePropertyChanged(QStringLiteral("SelectedDirectory"));
    }

    QList<Settings::DirectorySettings*> GameSelectorViewModel::enumerateDetectedGames()
    {
        // C# yields ~24 entries here, each probing a launcher install (Epic / Riot / Steam) or a registry
        // key (Rockstar, Level Infinite) for a known game. None of that scanning is ported — see the header
        // — so only the two entries that need no probing at all survive. They are kept because selecting
        // one is how a user discovers the live-service path is unsupported, rather than getting a confusing
        // "directory not found".
        QList<Settings::DirectorySettings*> games;
        games.append(Settings::DirectorySettings::Default(QStringLiteral("Fortnite [LIVE]"),
                                                          Constants::_FN_LIVE_TRIGGER, false,
                                                          CUE4Parse::UE4::Versions::GAME_UE5_8));
        games.append(Settings::DirectorySettings::Default(QStringLiteral("VALORANT [LIVE]"),
                                                          Constants::_VAL_LIVE_TRIGGER, false,
                                                          CUE4Parse::UE4::Versions::GAME_Valorant));
        for (Settings::DirectorySettings* game : games)
            game->setParent(this);
        return games;
    }

    void GameSelectorViewModel::addUndetectedDir(const QString& gameDirectory)
    {
        // C#'s SubstringAfterLast('\\'): a path with no backslash keeps its whole string as the name.
        const int slash = gameDirectory.lastIndexOf(QLatin1Char('\\'));
        addUndetectedDir(slash >= 0 ? gameDirectory.mid(slash + 1) : gameDirectory, gameDirectory);
    }

    void GameSelectorViewModel::addUndetectedDir(const QString& gameName, const QString& gameDirectory)
    {
        CUE4Parse::UE4::Versions::EGame ueVersion = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        QString newGameDirectory;
        // Upstream calls this for the version only: the line that would have swapped in the detected Paks
        // folder is commented out in the C# source, so the directory the user picked is what gets stored.
        tryDetectUeVersion(gameDirectory, ueVersion, newGameDirectory);

        auto* setting = Settings::DirectorySettings::Default(gameName, gameDirectory, true, ueVersion);
        setting->setParent(this);
        Settings::UserSettings::Default()->addPerDirectory(gameDirectory, setting->clone());
        _detectedDirectories.append(setting);
        setSelectedDirectory(_detectedDirectories.last());
    }

    void GameSelectorViewModel::deleteSelectedGame()
    {
        if (_selectedDirectory == nullptr)
            return;

        Settings::UserSettings::Default()->removePerDirectory(_selectedDirectory->gameDirectory());
        _detectedDirectories.removeOne(_selectedDirectory);
        delete _selectedDirectory;
        _selectedDirectory = nullptr;
        // C# selects DetectedDirectories.Last(), which throws on an empty list — it cannot be empty there
        // because the detected games are always present. Here it can, so an empty list selects nothing.
        setSelectedDirectory(_detectedDirectories.isEmpty() ? nullptr : _detectedDirectories.last());
    }

    bool GameSelectorViewModel::tryGetUeVersionFromExe(const QString& exePath,
                                                       CUE4Parse::UE4::Versions::EGame& ueVersion)
    {
        ueVersion = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
#ifdef _WIN32
        const std::wstring path = exePath.toStdWString();
        DWORD handle = 0;
        const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
        if (size == 0)
            return false;

        std::vector<uint8_t> buffer(size);
        if (!GetFileVersionInfoW(path.c_str(), handle, size, buffer.data()))
            return false;

        VS_FIXEDFILEINFO* info = nullptr;
        UINT infoSize = 0;
        if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<LPVOID*>(&info), &infoSize) || info == nullptr)
            return false;

        // C# reads FileVersionInfo.FileMajorPart / FileMinorPart, which are these two halves.
        const int major = static_cast<int>(HIWORD(info->dwFileVersionMS));
        const int minor = static_cast<int>(LOWORD(info->dwFileVersionMS));

        if (major == 4)
        {
            ueVersion = static_cast<CUE4Parse::UE4::Versions::EGame>(
                std::min<uint32_t>(CUE4Parse::UE4::Versions::GameUe4Base + (static_cast<uint32_t>(minor) << 16),
                                   static_cast<uint32_t>(CUE4Parse::UE4::Versions::GAME_UE4_LATEST)));
            return true;
        }
        if (major == 5)
        {
            ueVersion = static_cast<CUE4Parse::UE4::Versions::EGame>(
                std::min<uint32_t>(CUE4Parse::UE4::Versions::GameUe5Base + (static_cast<uint32_t>(minor) << 16),
                                   static_cast<uint32_t>(CUE4Parse::UE4::Versions::GAME_UE5_LATEST)));
            return true;
        }
        // C# throws for any other major version and the caller's catch turns it into `false`.
        return false;
#else
        (void)exePath;
        return false;
#endif
    }

    bool GameSelectorViewModel::tryDetectUeVersion(const QString& gameDirectory,
                                                   CUE4Parse::UE4::Versions::EGame& ueVersion,
                                                   QString& newGameDirectory)
    {
        ueVersion = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        QString targetGameDir = gameDirectory;

        if (!targetGameDir.endsWith(QStringLiteral("Paks"), Qt::CaseInsensitive))
        {
            QStringList paksDirs;
            QDirIterator it(targetGameDir, QStringList{QStringLiteral("Paks")}, QDir::Dirs | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);
            while (it.hasNext())
                paksDirs.append(QDir::toNativeSeparators(it.next()));

            QString paksDir;
            if (paksDirs.size() == 1)
            {
                paksDir = paksDirs.first();
            }
            else
            {
                // Every shipped UE game has an engine crash-reporter Paks folder; it is never the one wanted.
                for (const QString& candidate : paksDirs)
                {
                    if (!candidate.endsWith(QStringLiteral("Engine\\Programs\\CrashReportClient\\Content\\Paks")))
                    {
                        paksDir = candidate;
                        break;
                    }
                }
            }
            if (!paksDir.isEmpty())
                targetGameDir = paksDir;

            // The exe is looked for in the ORIGINAL directory (the BootstrapPackagedGame one), and only when
            // there is exactly one.
            const QStringList exes = QDir(gameDirectory).entryList(QStringList{QStringLiteral("*.exe")}, QDir::Files);
            if (exes.size() == 1 &&
                tryGetUeVersionFromExe(QDir(gameDirectory).filePath(exes.first()), ueVersion))
            {
                newGameDirectory = targetGameDir;
                return true;
            }
        }

        // past this point, we assume targetGameDir is the correct Paks folder
        newGameDirectory = targetGameDir;
        const QDir projectDir = QDir(QDir(targetGameDir).filePath(QStringLiteral("../..")));

        const QDir projectBinariesDir(projectDir.filePath(QStringLiteral("Binaries/Win64")));
        if (projectBinariesDir.exists())
        {
            const QStringList shipping =
                projectBinariesDir.entryList(QStringList{QStringLiteral("*-Win64-Shipping.exe")}, QDir::Files);
            if (!shipping.isEmpty())
            {
                for (const QString& exe : shipping)
                {
                    if (tryGetUeVersionFromExe(projectBinariesDir.filePath(exe), ueVersion))
                        return true;
                }
            }
            else
            {
                // C#'s `{ Length: < 3 }`: a folder with three or more loose exes is not a game's own.
                const QStringList exes = projectBinariesDir.entryList(QStringList{QStringLiteral("*.exe")}, QDir::Files);
                if (exes.size() < 3)
                {
                    for (const QString& exe : exes)
                    {
                        if (tryGetUeVersionFromExe(projectBinariesDir.filePath(exe), ueVersion))
                            return true;
                    }
                }
            }
        }

        const QString crashReportClientExe =
            projectDir.filePath(QStringLiteral("../Engine/Binaries/Win64/CrashReportClient.exe"));
        if (QFileInfo::exists(crashReportClientExe) && tryGetUeVersionFromExe(crashReportClientExe, ueVersion))
            return true;

        ueVersion = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        return false;
    }
}
