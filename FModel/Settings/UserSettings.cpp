#include "UserSettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include "DirectorySettings.h"
#include "EndpointSettings.h"
#include "../Extensions/JsonExtensions.h"

namespace FModel::Settings
{
    using namespace CUE4Parse::UE4::Lua::unluac;

    namespace
    {
        // C#'s `private static bool _bSave = true;` — Delete() latches it off so a Save() racing the app's
        // shutdown cannot recreate the file that was just deleted.
        bool g_bSave = true;

        UserSettings* g_default = nullptr;
    }

    UserSettings* UserSettings::Default()
    {
        // Stands in for C#'s static constructor (`Default = new UserSettings()`), but lazily, so this is safe
        // no matter which translation unit touches it first.
        if (!g_default)
            g_default = new UserSettings();

        return g_default;
    }

    void UserSettings::SetDefault(UserSettings* settings)
    {
        if (g_default == settings)
            return;

        delete g_default;
        g_default = settings;
    }

    QString UserSettings::FilePath()
    {
        const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        // AppDataLocation already ends in the organisation/application name under Qt, which is not what the C#
        // Environment.SpecialFolder.ApplicationData + "FModel" path is. GenericConfigLocation is the direct
        // equivalent of %APPDATA% on Windows.
        const QString roaming = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
        const QString base = roaming.isEmpty() ? appData : roaming;

#ifdef QT_DEBUG
        return QDir(base).filePath(QStringLiteral("FModel/AppSettings_Debug.json"));
#else
        return QDir(base).filePath(QStringLiteral("FModel/AppSettings.json"));
#endif
    }

    void UserSettings::Save()
    {
        if (!g_bSave || !g_default)
            return;

        if (DirectorySettings* current = g_default->currentDir())
        {
            // File the live directory back into the map before writing. Ownership transfers to the map here,
            // replacing whatever was stored for that path (which may be this very object — hence the guard).
            const QString key = current->gameDirectory();
            DirectorySettings* existing = g_default->_perDirectory.value(key, nullptr);
            if (existing != current)
            {
                delete existing;
                current->setParent(g_default);
                g_default->_perDirectory.insert(key, current);
            }
        }

        g_default->saveTo(FilePath());
    }

    void UserSettings::Delete()
    {
        const QString path = FilePath();
        if (QFile::exists(path))
        {
            g_bSave = false;
            QFile::remove(path);
        }
    }

    bool UserSettings::IsEndpointValid(EEndpointType type, EndpointSettings*& endpoint)
    {
        endpoint = nullptr;

        DirectorySettings* current = Default()->currentDir();
        if (!current)
            return false;

        const int index = static_cast<int>(type);
        if (index < 0 || index >= current->endpoints().size())
            return false;

        endpoint = current->endpoints().at(index);
        return endpoint->overwrite() || endpoint->isValid();
    }

    UserSettings::UserSettings(QObject* parent)
        : ViewModel(parent)
    {
        _nextUpdateCheck = QDateTime::currentDateTime();

        _dirLeftTab = new Hotkey(Key::A, ModifierKeys::None, this);
        _dirRightTab = new Hotkey(Key::D, ModifierKeys::None, this);
        _switchAssetExplorer = new Hotkey(Key::Z, ModifierKeys::None, this);
        _assetLeftTab = new Hotkey(Key::Q, ModifierKeys::None, this);
        _assetRightTab = new Hotkey(Key::E, ModifierKeys::None, this);
        _assetAddTab = new Hotkey(Key::T, ModifierKeys::Control, this);
        _assetRemoveTab = new Hotkey(Key::W, ModifierKeys::Control, this);
        _addAudio = new Hotkey(Key::N, ModifierKeys::Control, this);
        _playPauseAudio = new Hotkey(Key::K, ModifierKeys::None, this);
        _previousAudio = new Hotkey(Key::J, ModifierKeys::None, this);
        _nextAudio = new Hotkey(Key::L, ModifierKeys::None, this);
    }

    UserSettings::~UserSettings()
    {
        clearPerDirectory();
    }

    void UserSettings::clearPerDirectory()
    {
        qDeleteAll(_perDirectory);
        _perDirectory.clear();
        _currentDir = nullptr;
    }

    void UserSettings::setPerDirectory(const QHash<QString, DirectorySettings*>& value)
    {
        if (_perDirectory == value)
            return;

        clearPerDirectory();
        _perDirectory = value;
        for (DirectorySettings* settings : _perDirectory)
            settings->setParent(this);
        raisePropertyChanged(QStringLiteral("PerDirectory"));
    }

    void UserSettings::setManualGames(const QHash<QString, DetectedGame>& value)
    {
        _manualGames = value;
        raisePropertyChanged(QStringLiteral("ManualGames"));
    }

    void UserSettings::setLastAuthResponse(const AuthResponse& value)
    {
        _lastAuthResponse = value;
        raisePropertyChanged(QStringLiteral("LastAuthResponse"));
    }

    EUnluacMode UserSettings::unluacMode() const
    {
        return HasFlag(_unluacFlags, EUnluacFlags::Disassemble) ? EUnluacMode::Disassemble : EUnluacMode::Decompile;
    }

    void UserSettings::setUnluacMode(EUnluacMode value)
    {
        const EUnluacFlags withoutMode = _unluacFlags & ~(EUnluacFlags::Decompile | EUnluacFlags::Disassemble);
        const EUnluacFlags modeFlag = value == EUnluacMode::Disassemble ? EUnluacFlags::Disassemble : EUnluacFlags::Decompile;
        setUnluacFlags(withoutMode | modeFlag);
    }

    void UserSettings::setUnluacFlags(EUnluacFlags value)
    {
        if (!setProperty(_unluacFlags, value, QStringLiteral("UnluacFlags")))
            return;

        raisePropertyChanged(QStringLiteral("UnluacMode"));
    }

    QJsonObject UserSettings::toJson() const
    {
        using namespace FModel::Extensions;

        QJsonObject json;
        json[QStringLiteral("ShowChangelog")] = _showChangelog;
        json[QStringLiteral("OutputDirectory")] = _outputDirectory;
        json[QStringLiteral("RawDataDirectory")] = _rawDataDirectory;
        json[QStringLiteral("PropertiesDirectory")] = _propertiesDirectory;
        json[QStringLiteral("TextureDirectory")] = _textureDirectory;
        json[QStringLiteral("AudioDirectory")] = _audioDirectory;
        json[QStringLiteral("CodeDirectory")] = _codeDirectory;
        json[QStringLiteral("ModelDirectory")] = _modelDirectory;
        json[QStringLiteral("GameDirectory")] = _gameDirectory;
        json[QStringLiteral("LastOpenedSettingTab")] = _lastOpenedSettingTab;
        json[QStringLiteral("IsLoggerExpanded")] = _isLoggerExpanded;
        json[QStringLiteral("AvalonImageSize")] = _avalonImageSize.toString();
        json[QStringLiteral("AudioDeviceId")] = _audioDeviceId;
        json[QStringLiteral("AudioPlayerVolume")] = static_cast<double>(_audioPlayerVolume);
        json[QStringLiteral("LoadingMode")] = enumToJson(_loadingMode);
        json[QStringLiteral("LastUpdateCheck")] = FModel::Extensions::toJson(_lastUpdateCheck);
        json[QStringLiteral("NextUpdateCheck")] = FModel::Extensions::toJson(_nextUpdateCheck);
        json[QStringLiteral("KeepDirectoryStructure")] = _keepDirectoryStructure;
        json[QStringLiteral("ShowDecompileOption")] = _showDecompileOption;
        json[QStringLiteral("CompressedAudioMode")] = enumToJson(_compressedAudioMode);
        json[QStringLiteral("AesReload")] = enumToJson(_aesReload);
        json[QStringLiteral("DiscordRpc")] = enumToJson(_discordRpc);
        json[QStringLiteral("AssetLanguage")] = enumToJson(_assetLanguage);
        json[QStringLiteral("CosmeticStyle")] = enumToJson(_cosmeticStyle);
        json[QStringLiteral("CosmeticDisplayAsset")] = _cosmeticDisplayAsset;
        json[QStringLiteral("ImageMergerMargin")] = _imageMergerMargin;
        json[QStringLiteral("ReadScriptData")] = _readScriptData;
        json[QStringLiteral("ReadShaderMaps")] = _readShaderMaps;
        json[QStringLiteral("ConvertAudioOnBulkExport")] = _convertAudioOnBulkExport;
        json[QStringLiteral("DecompileLua")] = _decompileLua;
        json[QStringLiteral("UnluacFlags")] = enumToJson(_unluacFlags);
        json[QStringLiteral("JsonHighlightTheme")] = enumToJson(_jsonHighlightTheme);

        QJsonObject perDirectory;
        for (auto it = _perDirectory.constBegin(); it != _perDirectory.constEnd(); ++it)
            perDirectory[it.key()] = it.value()->toJson();
        json[QStringLiteral("PerDirectory")] = perDirectory;

        QJsonObject manualGames;
        for (auto it = _manualGames.constBegin(); it != _manualGames.constEnd(); ++it)
            manualGames[it.key()] = it.value().toJson();
        json[QStringLiteral("ManualGames")] = manualGames;

        json[QStringLiteral("LastAuthResponse")] = _lastAuthResponse.toJson();

        json[QStringLiteral("DirLeftTab")] = _dirLeftTab->toJson();
        json[QStringLiteral("DirRightTab")] = _dirRightTab->toJson();
        json[QStringLiteral("SwitchAssetExplorer")] = _switchAssetExplorer->toJson();
        json[QStringLiteral("AssetLeftTab")] = _assetLeftTab->toJson();
        json[QStringLiteral("AssetRightTab")] = _assetRightTab->toJson();
        json[QStringLiteral("AssetAddTab")] = _assetAddTab->toJson();
        json[QStringLiteral("AssetRemoveTab")] = _assetRemoveTab->toJson();
        json[QStringLiteral("AddAudio")] = _addAudio->toJson();
        json[QStringLiteral("PlayPauseAudio")] = _playPauseAudio->toJson();
        json[QStringLiteral("PreviousAudio")] = _previousAudio->toJson();
        json[QStringLiteral("NextAudio")] = _nextAudio->toJson();

        json[QStringLiteral("MeshExportFormat")] = enumToJson(_meshExportFormat);
        json[QStringLiteral("NaniteMeshExportFormat")] = enumToJson(_naniteMeshExportFormat);
        json[QStringLiteral("MaterialExportFormat")] = enumToJson(_materialExportFormat);
        json[QStringLiteral("TextureExportFormat")] = enumToJson(_textureExportFormat);
        json[QStringLiteral("SocketExportFormat")] = enumToJson(_socketExportFormat);
        json[QStringLiteral("CompressionFormat")] = enumToJson(_compressionFormat);
        json[QStringLiteral("LodExportFormat")] = enumToJson(_lodExportFormat);

        json[QStringLiteral("ShowSkybox")] = _showSkybox;
        json[QStringLiteral("ShowGrid")] = _showGrid;
        json[QStringLiteral("AnimateWithRotationOnly")] = _animateWithRotationOnly;
        json[QStringLiteral("CameraMode")] = enumToJson(_cameraMode);
        json[QStringLiteral("PreviewMaxTextureSize")] = _previewMaxTextureSize;
        json[QStringLiteral("PreviewStaticMeshes")] = _previewStaticMeshes;
        json[QStringLiteral("PreviewSkeletalMeshes")] = _previewSkeletalMeshes;
        json[QStringLiteral("PreviewAnimations")] = _previewAnimations;
        json[QStringLiteral("PreviewMaterials")] = _previewMaterials;
        json[QStringLiteral("PreviewWorlds")] = _previewWorlds;
        json[QStringLiteral("SaveMorphTargets")] = _saveMorphTargets;
        json[QStringLiteral("SaveEmbeddedMaterials")] = _saveEmbeddedMaterials;
        json[QStringLiteral("SaveSkeletonAsMesh")] = _saveSkeletonAsMesh;
        json[QStringLiteral("SaveHdrTexturesAsHdr")] = _saveHdrTexturesAsHdr;
        json[QStringLiteral("FeaturePreviewNewAssetExplorer")] = _featurePreviewNewAssetExplorer;
        json[QStringLiteral("PreviewTexturesAssetExplorer")] = _previewTexturesAssetExplorer;

        return json;
    }

    void UserSettings::readJson(const QJsonObject& json)
    {
        using namespace FModel::Extensions;

        setShowChangelog(boolFromJson(json, QStringLiteral("ShowChangelog"), _showChangelog));
        setOutputDirectory(stringFromJson(json, QStringLiteral("OutputDirectory"), _outputDirectory));
        setRawDataDirectory(stringFromJson(json, QStringLiteral("RawDataDirectory"), _rawDataDirectory));
        setPropertiesDirectory(stringFromJson(json, QStringLiteral("PropertiesDirectory"), _propertiesDirectory));
        setTextureDirectory(stringFromJson(json, QStringLiteral("TextureDirectory"), _textureDirectory));
        setAudioDirectory(stringFromJson(json, QStringLiteral("AudioDirectory"), _audioDirectory));
        setCodeDirectory(stringFromJson(json, QStringLiteral("CodeDirectory"), _codeDirectory));
        setModelDirectory(stringFromJson(json, QStringLiteral("ModelDirectory"), _modelDirectory));
        setGameDirectory(stringFromJson(json, QStringLiteral("GameDirectory"), _gameDirectory));
        setLastOpenedSettingTab(intFromJson(json, QStringLiteral("LastOpenedSettingTab"), _lastOpenedSettingTab));
        setIsLoggerExpanded(boolFromJson(json, QStringLiteral("IsLoggerExpanded"), _isLoggerExpanded));

        if (json.value(QStringLiteral("AvalonImageSize")).isString())
            setAvalonImageSize(GridLength::fromString(json[QStringLiteral("AvalonImageSize")].toString()));

        setAudioDeviceId(stringFromJson(json, QStringLiteral("AudioDeviceId"), _audioDeviceId));
        setAudioPlayerVolume(static_cast<float>(doubleFromJson(json, QStringLiteral("AudioPlayerVolume"), _audioPlayerVolume)));
        setLoadingMode(enumFromJson(json, QStringLiteral("LoadingMode"), _loadingMode));
        setLastUpdateCheck(dateTimeFromJson(json[QStringLiteral("LastUpdateCheck")], _lastUpdateCheck));
        setNextUpdateCheck(dateTimeFromJson(json[QStringLiteral("NextUpdateCheck")], _nextUpdateCheck));
        setKeepDirectoryStructure(boolFromJson(json, QStringLiteral("KeepDirectoryStructure"), _keepDirectoryStructure));
        setShowDecompileOption(boolFromJson(json, QStringLiteral("ShowDecompileOption"), _showDecompileOption));
        setCompressedAudioMode(enumFromJson(json, QStringLiteral("CompressedAudioMode"), _compressedAudioMode));
        setAesReload(enumFromJson(json, QStringLiteral("AesReload"), _aesReload));
        setDiscordRpc(enumFromJson(json, QStringLiteral("DiscordRpc"), _discordRpc));
        setAssetLanguage(enumFromJson(json, QStringLiteral("AssetLanguage"), _assetLanguage));
        setCosmeticStyle(enumFromJson(json, QStringLiteral("CosmeticStyle"), _cosmeticStyle));
        setCosmeticDisplayAsset(boolFromJson(json, QStringLiteral("CosmeticDisplayAsset"), _cosmeticDisplayAsset));
        setImageMergerMargin(intFromJson(json, QStringLiteral("ImageMergerMargin"), _imageMergerMargin));
        setReadScriptData(boolFromJson(json, QStringLiteral("ReadScriptData"), _readScriptData));
        setReadShaderMaps(boolFromJson(json, QStringLiteral("ReadShaderMaps"), _readShaderMaps));
        setConvertAudioOnBulkExport(boolFromJson(json, QStringLiteral("ConvertAudioOnBulkExport"), _convertAudioOnBulkExport));
        setDecompileLua(boolFromJson(json, QStringLiteral("DecompileLua"), _decompileLua));
        setUnluacFlags(enumFromJson(json, QStringLiteral("UnluacFlags"), _unluacFlags));
        setJsonHighlightTheme(enumFromJson(json, QStringLiteral("JsonHighlightTheme"), _jsonHighlightTheme));

        if (json.contains(QStringLiteral("PerDirectory")))
        {
            QHash<QString, DirectorySettings*> perDirectory;
            const QJsonObject stored = json[QStringLiteral("PerDirectory")].toObject();
            for (auto it = stored.constBegin(); it != stored.constEnd(); ++it)
                perDirectory.insert(it.key(), DirectorySettings::fromJson(it.value().toObject()));
            setPerDirectory(perDirectory);
        }

        if (json.contains(QStringLiteral("ManualGames")))
        {
            QHash<QString, DetectedGame> manualGames;
            const QJsonObject stored = json[QStringLiteral("ManualGames")].toObject();
            for (auto it = stored.constBegin(); it != stored.constEnd(); ++it)
                manualGames.insert(it.key(), DetectedGame::fromJson(it.value().toObject()));
            setManualGames(manualGames);
        }

        if (json.contains(QStringLiteral("LastAuthResponse")))
            setLastAuthResponse(AuthResponse::fromJson(json[QStringLiteral("LastAuthResponse")].toObject()));

        const auto readHotkey = [&json](const QString& key, Hotkey* hotkey)
        {
            if (json.contains(key))
                hotkey->readJson(json[key].toObject());
        };
        readHotkey(QStringLiteral("DirLeftTab"), _dirLeftTab);
        readHotkey(QStringLiteral("DirRightTab"), _dirRightTab);
        readHotkey(QStringLiteral("SwitchAssetExplorer"), _switchAssetExplorer);
        readHotkey(QStringLiteral("AssetLeftTab"), _assetLeftTab);
        readHotkey(QStringLiteral("AssetRightTab"), _assetRightTab);
        readHotkey(QStringLiteral("AssetAddTab"), _assetAddTab);
        readHotkey(QStringLiteral("AssetRemoveTab"), _assetRemoveTab);
        readHotkey(QStringLiteral("AddAudio"), _addAudio);
        readHotkey(QStringLiteral("PlayPauseAudio"), _playPauseAudio);
        readHotkey(QStringLiteral("PreviousAudio"), _previousAudio);
        readHotkey(QStringLiteral("NextAudio"), _nextAudio);

        setMeshExportFormat(enumFromJson(json, QStringLiteral("MeshExportFormat"), _meshExportFormat));
        setNaniteMeshExportFormat(enumFromJson(json, QStringLiteral("NaniteMeshExportFormat"), _naniteMeshExportFormat));
        setMaterialExportFormat(enumFromJson(json, QStringLiteral("MaterialExportFormat"), _materialExportFormat));
        setTextureExportFormat(enumFromJson(json, QStringLiteral("TextureExportFormat"), _textureExportFormat));
        setSocketExportFormat(enumFromJson(json, QStringLiteral("SocketExportFormat"), _socketExportFormat));
        setCompressionFormat(enumFromJson(json, QStringLiteral("CompressionFormat"), _compressionFormat));
        setLodExportFormat(enumFromJson(json, QStringLiteral("LodExportFormat"), _lodExportFormat));

        setShowSkybox(boolFromJson(json, QStringLiteral("ShowSkybox"), _showSkybox));
        setShowGrid(boolFromJson(json, QStringLiteral("ShowGrid"), _showGrid));
        setAnimateWithRotationOnly(boolFromJson(json, QStringLiteral("AnimateWithRotationOnly"), _animateWithRotationOnly));
        setCameraMode(enumFromJson(json, QStringLiteral("CameraMode"), _cameraMode));
        setPreviewMaxTextureSize(intFromJson(json, QStringLiteral("PreviewMaxTextureSize"), _previewMaxTextureSize));
        setPreviewStaticMeshes(boolFromJson(json, QStringLiteral("PreviewStaticMeshes"), _previewStaticMeshes));
        setPreviewSkeletalMeshes(boolFromJson(json, QStringLiteral("PreviewSkeletalMeshes"), _previewSkeletalMeshes));
        setPreviewAnimations(boolFromJson(json, QStringLiteral("PreviewAnimations"), _previewAnimations));
        setPreviewMaterials(boolFromJson(json, QStringLiteral("PreviewMaterials"), _previewMaterials));
        setPreviewWorlds(boolFromJson(json, QStringLiteral("PreviewWorlds"), _previewWorlds));
        setSaveMorphTargets(boolFromJson(json, QStringLiteral("SaveMorphTargets"), _saveMorphTargets));
        setSaveEmbeddedMaterials(boolFromJson(json, QStringLiteral("SaveEmbeddedMaterials"), _saveEmbeddedMaterials));
        setSaveSkeletonAsMesh(boolFromJson(json, QStringLiteral("SaveSkeletonAsMesh"), _saveSkeletonAsMesh));
        setSaveHdrTexturesAsHdr(boolFromJson(json, QStringLiteral("SaveHdrTexturesAsHdr"), _saveHdrTexturesAsHdr));
        setFeaturePreviewNewAssetExplorer(boolFromJson(json, QStringLiteral("FeaturePreviewNewAssetExplorer"), _featurePreviewNewAssetExplorer));
        setPreviewTexturesAssetExplorer(boolFromJson(json, QStringLiteral("PreviewTexturesAssetExplorer"), _previewTexturesAssetExplorer));
    }

    bool UserSettings::saveTo(const QString& filePath) const
    {
        QDir().mkpath(QFileInfo(filePath).absolutePath());

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;

        // Formatting.Indented in C#.
        const QJsonDocument document(toJson());
        return file.write(document.toJson(QJsonDocument::Indented)) != -1;
    }

    UserSettings* UserSettings::loadFrom(const QString& filePath, QObject* parent)
    {
        auto* settings = new UserSettings(parent);

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            return settings; // C# leaves Default at its constructed values when the file is missing.

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (document.isObject())
            settings->readJson(document.object());

        return settings;
    }
}
