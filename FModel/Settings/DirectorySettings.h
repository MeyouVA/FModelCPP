#pragma once
// Ported from FModel/Settings/DirectorySettings.cs — the per-game-directory settings block, keyed by game
// directory inside UserSettings::PerDirectory.
//
// Ownership: Versioning, Endpoints and Directories are heap objects parented to this DirectorySettings, so a
// DirectorySettings owns its whole subtree and destroying it destroys the lot.

#include "../Framework/ViewModel.h"
#include "../ViewModels/ApiEndpoints/Models/AesResponse.h"

#include <QDateTime>
#include <QList>
#include <QString>

#include "UE4/Assets/Exports/Texture/ETexturePlatform.h"
#include "UE4/Versions/EGame.h"

class QJsonObject;

namespace FModel::Settings
{
    class CustomDirectory;
    class EndpointSettings;
    class VersioningSettings;

    class DirectorySettings : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        using EGame = CUE4Parse::UE4::Versions::EGame;
        using ETexturePlatform = CUE4Parse::UE4::Assets::Exports::Texture::ETexturePlatform;
        using AesResponse = ViewModels::ApiEndpoints::Models::AesResponse;

        // Builds the settings for a directory, reusing whatever UserSettings already has stored for that same
        // path so re-detecting a game never discards the user's tweaks.
        static DirectorySettings* Default(const QString& gameName, const QString& gameDir, bool manual = false,
                                          EGame ue = CUE4Parse::UE4::Versions::GAME_UE4_LATEST,
                                          const QString& aes = QString(), QObject* parent = nullptr);

        explicit DirectorySettings(QObject* parent = nullptr);

        const QString& gameName() const { return _gameName; }
        void setGameName(const QString& value) { setProperty(_gameName, value, QStringLiteral("GameName")); }

        const QString& gameDirectory() const { return _gameDirectory; }
        void setGameDirectory(const QString& value) { setProperty(_gameDirectory, value, QStringLiteral("GameDirectory")); }

        bool isManual() const { return _isManual; }
        void setIsManual(bool value) { setProperty(_isManual, value, QStringLiteral("IsManual")); }

        EGame ueVersion() const { return _ueVersion; }
        void setUeVersion(EGame value) { setProperty(_ueVersion, value, QStringLiteral("UeVersion")); }

        ETexturePlatform texturePlatform() const { return _texturePlatform; }
        void setTexturePlatform(ETexturePlatform value) { setProperty(_texturePlatform, value, QStringLiteral("TexturePlatform")); }

        VersioningSettings* versioning() const { return _versioning; }
        void setVersioning(VersioningSettings* value);

        const QList<EndpointSettings*>& endpoints() const { return _endpoints; }
        void setEndpoints(const QList<EndpointSettings*>& value);

        const QList<CustomDirectory*>& directories() const { return _directories; }
        void setDirectories(const QList<CustomDirectory*>& value);

        const AesResponse& aesKeys() const { return _aesKeys; }
        void setAesKeys(const AesResponse& value);

        const QDateTime& lastAesReload() const { return _lastAesReload; }
        void setLastAesReload(const QDateTime& value) { setProperty(_lastAesReload, value, QStringLiteral("LastAesReload")); }

        quint64 criwareDecryptionKey() const { return _criwareDecryptionKey; }
        void setCriwareDecryptionKey(quint64 value) { setProperty(_criwareDecryptionKey, value, QStringLiteral("CriwareDecryptionKey")); }

        const QString& unluacOpCodeMap() const { return _unluacOpCodeMap; }
        void setUnluacOpCodeMap(const QString& value) { setProperty(_unluacOpCodeMap, value, QStringLiteral("UnluacOpCodeMap")); }

        // C# equality is deliberately narrow: directory + engine version only.
        bool equals(const DirectorySettings& other) const
        {
            return _gameDirectory == other._gameDirectory && _ueVersion == other._ueVersion;
        }

        QString toString() const { return _gameName; }

        // C#'s ICloneable.Clone() is MemberwiseClone(), i.e. a SHALLOW copy: the clone shares its Versioning,
        // Endpoints, Directories and AesKeys instances with the original. This port deep-copies instead,
        // because the C++ objects are owned by their parent and sharing them across two owners would mean
        // either double-delete or dangling. The visible difference: editing a clone's endpoint list no longer
        // writes through to the original. GameSelectorViewModel — the only caller — treats its clones as
        // independent rows, so it wants the deep copy.
        DirectorySettings* clone(QObject* parent = nullptr) const;

        QJsonObject toJson() const;
        void readJson(const QJsonObject& json);
        static DirectorySettings* fromJson(const QJsonObject& json, QObject* parent = nullptr);

    private:
        void clearEndpoints();
        void clearDirectories();

        QString _gameName;
        QString _gameDirectory;
        bool _isManual = false;
        EGame _ueVersion = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        ETexturePlatform _texturePlatform = ETexturePlatform::DesktopMobile;
        VersioningSettings* _versioning = nullptr;
        QList<EndpointSettings*> _endpoints;
        QList<CustomDirectory*> _directories;
        AesResponse _aesKeys;
        QDateTime _lastAesReload;
        quint64 _criwareDecryptionKey = 0;
        QString _unluacOpCodeMap;
    };
}
