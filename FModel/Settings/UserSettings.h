#pragma once
// Ported from FModel/Settings/UserSettings.cs — the root of the persisted settings tree.
//
// Deferred:
//   * ExportOptions — projects this object onto CUE4Parse-Conversion's ExporterOptions, which is not ported.
//     Every field it reads is here, so it is a few lines once the exporters land.
//   * IsEndpointValid's callers all live in the API-endpoint view-models; the method itself IS ported, since
//     it only reads settings.
//
// Serialisation notes (see Extensions/JsonExtensions.h for the general rules): property names are PascalCase,
// matching Newtonsoft's default contract — except inside AesResponse/AuthResponse, which pin camelCase and
// snake_case names respectively because they double as wire formats.

#include "../Framework/GridLength.h"
#include "../Framework/Hotkey.h"
#include "../Framework/ViewModel.h"
#include "../Enums.h"
#include "../Extensions/Themes/JsonHighlightThemes.h"
#include "../Views/Snooper/Camera.h"
#include "../ViewModels/ApiEndpoints/Models/EpicResponse.h"
#include "../ViewModels/GameSelectorViewModel.h"

#include <QDateTime>
#include <QHash>
#include <QString>

#include "UE4/Assets/Exports/Material/EMaterialFormat.h"
#include "UE4/Assets/Exports/Nanite/ENaniteMeshFormat.h"
#include "UE4/Lua/unluac/EUnluacFlags.h"
#include "UE4/Versions/ELanguage.h"

#include "Animations/EAnimFormat.h"
#include "Meshes/ELodFormat.h"
#include "Meshes/EMeshFormat.h"
#include "Meshes/ESocketFormat.h"
#include "Textures/ETextureFormat.h"
#include "UEFormat/Enums/EFileCompressionFormat.h"

class QJsonObject;

namespace FModel::Settings
{
    class DirectorySettings;
    class EndpointSettings;

    class UserSettings : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        using EUnluacFlags = CUE4Parse::UE4::Lua::unluac::EUnluacFlags;
        using ELanguage = CUE4Parse::UE4::Versions::ELanguage;
        using EMaterialFormat = CUE4Parse::UE4::Assets::Exports::Material::EMaterialFormat;
        using ENaniteMeshFormat = CUE4Parse::UE4::Assets::Exports::Nanite::ENaniteMeshFormat;
        using EAnimFormat = CUE4Parse_Conversion::Animations::EAnimFormat;
        using ELodFormat = CUE4Parse_Conversion::Meshes::ELodFormat;
        using EMeshFormat = CUE4Parse_Conversion::Meshes::EMeshFormat;
        using ESocketFormat = CUE4Parse_Conversion::Meshes::ESocketFormat;
        using ETextureFormat = CUE4Parse_Conversion::Textures::ETextureFormat;
        using EFileCompressionFormat = CUE4Parse_Conversion::UEFormat::Enums::EFileCompressionFormat;
        using EJsonHighlightTheme = Extensions::Themes::EJsonHighlightTheme;
        using WorldMode = Views::Snooper::Camera::WorldMode;
        using AuthResponse = ViewModels::ApiEndpoints::Models::AuthResponse;
        using DetectedGame = ViewModels::DetectedGame;
        using Hotkey = Framework::Hotkey;
        using GridLength = Framework::GridLength;
        using Key = Framework::Key;
        using ModifierKeys = Framework::ModifierKeys;

        // C#'s `public static UserSettings Default { get; set; }`, initialised by a static constructor.
        // Returns a lazily created singleton so there is no static-initialisation-order hazard.
        static UserSettings* Default();
        static void SetDefault(UserSettings* settings);

        // %APPDATA%/FModel/AppSettings.json — AppSettings_Debug.json in a debug build, exactly as the C#
        // #if DEBUG does, so a debug session can never scribble on a release install's settings.
        static QString FilePath();

        static void Save();
        static void Delete();

        // Endpoints are indexed by EEndpointType, so the enum's value is the array index.
        static bool IsEndpointValid(EEndpointType type, EndpointSettings*& endpoint);

        explicit UserSettings(QObject* parent = nullptr);
        ~UserSettings() override;

        // --- persisted scalars -------------------------------------------------------------------------
        bool showChangelog() const { return _showChangelog; }
        void setShowChangelog(bool v) { setProperty(_showChangelog, v, QStringLiteral("ShowChangelog")); }

        const QString& outputDirectory() const { return _outputDirectory; }
        void setOutputDirectory(const QString& v) { setProperty(_outputDirectory, v, QStringLiteral("OutputDirectory")); }

        const QString& rawDataDirectory() const { return _rawDataDirectory; }
        void setRawDataDirectory(const QString& v) { setProperty(_rawDataDirectory, v, QStringLiteral("RawDataDirectory")); }

        const QString& propertiesDirectory() const { return _propertiesDirectory; }
        void setPropertiesDirectory(const QString& v) { setProperty(_propertiesDirectory, v, QStringLiteral("PropertiesDirectory")); }

        const QString& textureDirectory() const { return _textureDirectory; }
        void setTextureDirectory(const QString& v) { setProperty(_textureDirectory, v, QStringLiteral("TextureDirectory")); }

        const QString& audioDirectory() const { return _audioDirectory; }
        void setAudioDirectory(const QString& v) { setProperty(_audioDirectory, v, QStringLiteral("AudioDirectory")); }

        const QString& codeDirectory() const { return _codeDirectory; }
        void setCodeDirectory(const QString& v) { setProperty(_codeDirectory, v, QStringLiteral("CodeDirectory")); }

        const QString& modelDirectory() const { return _modelDirectory; }
        void setModelDirectory(const QString& v) { setProperty(_modelDirectory, v, QStringLiteral("ModelDirectory")); }

        const QString& gameDirectory() const { return _gameDirectory; }
        void setGameDirectory(const QString& v) { setProperty(_gameDirectory, v, QStringLiteral("GameDirectory")); }

        int lastOpenedSettingTab() const { return _lastOpenedSettingTab; }
        void setLastOpenedSettingTab(int v) { setProperty(_lastOpenedSettingTab, v, QStringLiteral("LastOpenedSettingTab")); }

        bool isLoggerExpanded() const { return _isLoggerExpanded; }
        void setIsLoggerExpanded(bool v) { setProperty(_isLoggerExpanded, v, QStringLiteral("IsLoggerExpanded")); }

        const GridLength& avalonImageSize() const { return _avalonImageSize; }
        void setAvalonImageSize(const GridLength& v) { setProperty(_avalonImageSize, v, QStringLiteral("AvalonImageSize")); }

        const QString& audioDeviceId() const { return _audioDeviceId; }
        void setAudioDeviceId(const QString& v) { setProperty(_audioDeviceId, v, QStringLiteral("AudioDeviceId")); }

        float audioPlayerVolume() const { return _audioPlayerVolume; }
        void setAudioPlayerVolume(float v) { setProperty(_audioPlayerVolume, v, QStringLiteral("AudioPlayerVolume")); }

        ELoadingMode loadingMode() const { return _loadingMode; }
        void setLoadingMode(ELoadingMode v) { setProperty(_loadingMode, v, QStringLiteral("LoadingMode")); }

        const QDateTime& lastUpdateCheck() const { return _lastUpdateCheck; }
        void setLastUpdateCheck(const QDateTime& v) { setProperty(_lastUpdateCheck, v, QStringLiteral("LastUpdateCheck")); }

        const QDateTime& nextUpdateCheck() const { return _nextUpdateCheck; }
        void setNextUpdateCheck(const QDateTime& v) { setProperty(_nextUpdateCheck, v, QStringLiteral("NextUpdateCheck")); }

        bool keepDirectoryStructure() const { return _keepDirectoryStructure; }
        void setKeepDirectoryStructure(bool v) { setProperty(_keepDirectoryStructure, v, QStringLiteral("KeepDirectoryStructure")); }

        bool showDecompileOption() const { return _showDecompileOption; }
        void setShowDecompileOption(bool v) { setProperty(_showDecompileOption, v, QStringLiteral("ShowDecompileOption")); }

        ECompressedAudio compressedAudioMode() const { return _compressedAudioMode; }
        void setCompressedAudioMode(ECompressedAudio v) { setProperty(_compressedAudioMode, v, QStringLiteral("CompressedAudioMode")); }

        EAesReload aesReload() const { return _aesReload; }
        void setAesReload(EAesReload v) { setProperty(_aesReload, v, QStringLiteral("AesReload")); }

        EDiscordRpc discordRpc() const { return _discordRpc; }
        void setDiscordRpc(EDiscordRpc v) { setProperty(_discordRpc, v, QStringLiteral("DiscordRpc")); }

        ELanguage assetLanguage() const { return _assetLanguage; }
        void setAssetLanguage(ELanguage v) { setProperty(_assetLanguage, v, QStringLiteral("AssetLanguage")); }

        EIconStyle cosmeticStyle() const { return _cosmeticStyle; }
        void setCosmeticStyle(EIconStyle v) { setProperty(_cosmeticStyle, v, QStringLiteral("CosmeticStyle")); }

        bool cosmeticDisplayAsset() const { return _cosmeticDisplayAsset; }
        void setCosmeticDisplayAsset(bool v) { setProperty(_cosmeticDisplayAsset, v, QStringLiteral("CosmeticDisplayAsset")); }

        int imageMergerMargin() const { return _imageMergerMargin; }
        void setImageMergerMargin(int v) { setProperty(_imageMergerMargin, v, QStringLiteral("ImageMergerMargin")); }

        bool readScriptData() const { return _readScriptData; }
        void setReadScriptData(bool v) { setProperty(_readScriptData, v, QStringLiteral("ReadScriptData")); }

        bool readShaderMaps() const { return _readShaderMaps; }
        void setReadShaderMaps(bool v) { setProperty(_readShaderMaps, v, QStringLiteral("ReadShaderMaps")); }

        bool convertAudioOnBulkExport() const { return _convertAudioOnBulkExport; }
        void setConvertAudioOnBulkExport(bool v) { setProperty(_convertAudioOnBulkExport, v, QStringLiteral("ConvertAudioOnBulkExport")); }

        bool decompileLua() const { return _decompileLua; }
        void setDecompileLua(bool v) { setProperty(_decompileLua, v, QStringLiteral("DecompileLua")); }

        // [JsonIgnore] — a view over UnluacFlags, not storage of its own.
        EUnluacMode unluacMode() const;
        void setUnluacMode(EUnluacMode value);

        EUnluacFlags unluacFlags() const { return _unluacFlags; }
        void setUnluacFlags(EUnluacFlags value);

        EJsonHighlightTheme jsonHighlightTheme() const { return _jsonHighlightTheme; }
        void setJsonHighlightTheme(EJsonHighlightTheme v) { setProperty(_jsonHighlightTheme, v, QStringLiteral("JsonHighlightTheme")); }

        // --- per-directory settings --------------------------------------------------------------------
        const QHash<QString, DirectorySettings*>& perDirectory() const { return _perDirectory; }
        void setPerDirectory(const QHash<QString, DirectorySettings*>& value);
        // C# indexes the dictionary directly (`PerDirectory[dir] = setting` / `.Remove(dir)`); these two
        // exist because the map owns its values here, so a replaced or removed entry has to be deleted.
        DirectorySettings* perDirectory(const QString& gameDirectory) const
        { return _perDirectory.value(gameDirectory, nullptr); }
        void addPerDirectory(const QString& gameDirectory, DirectorySettings* settings);
        void removePerDirectory(const QString& gameDirectory);

        // [JsonIgnore] — the directory currently loaded. Not owned: it is an entry of PerDirectory, or a
        // freshly built DirectorySettings that Save() then files into PerDirectory.
        DirectorySettings* currentDir() const { return _currentDir; }
        void setCurrentDir(DirectorySettings* value) { _currentDir = value; }

        const QHash<QString, DetectedGame>& manualGames() const { return _manualGames; }
        void setManualGames(const QHash<QString, DetectedGame>& value);

        const AuthResponse& lastAuthResponse() const { return _lastAuthResponse; }
        void setLastAuthResponse(const AuthResponse& value);

        // --- hotkeys -----------------------------------------------------------------------------------
        Hotkey* dirLeftTab() const { return _dirLeftTab; }
        Hotkey* dirRightTab() const { return _dirRightTab; }
        Hotkey* switchAssetExplorer() const { return _switchAssetExplorer; }
        Hotkey* assetLeftTab() const { return _assetLeftTab; }
        Hotkey* assetRightTab() const { return _assetRightTab; }
        Hotkey* assetAddTab() const { return _assetAddTab; }
        Hotkey* assetRemoveTab() const { return _assetRemoveTab; }
        Hotkey* addAudio() const { return _addAudio; }
        Hotkey* playPauseAudio() const { return _playPauseAudio; }
        Hotkey* previousAudio() const { return _previousAudio; }
        Hotkey* nextAudio() const { return _nextAudio; }

        // --- export formats ----------------------------------------------------------------------------
        EMeshFormat meshExportFormat() const { return _meshExportFormat; }
        void setMeshExportFormat(EMeshFormat v) { setProperty(_meshExportFormat, v, QStringLiteral("MeshExportFormat")); }

        ENaniteMeshFormat naniteMeshExportFormat() const { return _naniteMeshExportFormat; }
        void setNaniteMeshExportFormat(ENaniteMeshFormat v) { setProperty(_naniteMeshExportFormat, v, QStringLiteral("NaniteMeshExportFormat")); }

        EMaterialFormat materialExportFormat() const { return _materialExportFormat; }
        void setMaterialExportFormat(EMaterialFormat v) { setProperty(_materialExportFormat, v, QStringLiteral("MaterialExportFormat")); }

        ETextureFormat textureExportFormat() const { return _textureExportFormat; }
        void setTextureExportFormat(ETextureFormat v) { setProperty(_textureExportFormat, v, QStringLiteral("TextureExportFormat")); }

        ESocketFormat socketExportFormat() const { return _socketExportFormat; }
        void setSocketExportFormat(ESocketFormat v) { setProperty(_socketExportFormat, v, QStringLiteral("SocketExportFormat")); }

        EFileCompressionFormat compressionFormat() const { return _compressionFormat; }
        void setCompressionFormat(EFileCompressionFormat v) { setProperty(_compressionFormat, v, QStringLiteral("CompressionFormat")); }

        ELodFormat lodExportFormat() const { return _lodExportFormat; }
        void setLodExportFormat(ELodFormat v) { setProperty(_lodExportFormat, v, QStringLiteral("LodExportFormat")); }

        // The anim format is not stored: ExportOptions derives it from the mesh format.
        EAnimFormat animExportFormat() const
        {
            return _meshExportFormat == EMeshFormat::UEFormat ? EAnimFormat::UEFormat : EAnimFormat::ActorX;
        }

        // --- viewer ------------------------------------------------------------------------------------
        bool showSkybox() const { return _showSkybox; }
        void setShowSkybox(bool v) { setProperty(_showSkybox, v, QStringLiteral("ShowSkybox")); }

        bool showGrid() const { return _showGrid; }
        void setShowGrid(bool v) { setProperty(_showGrid, v, QStringLiteral("ShowGrid")); }

        bool animateWithRotationOnly() const { return _animateWithRotationOnly; }
        void setAnimateWithRotationOnly(bool v) { setProperty(_animateWithRotationOnly, v, QStringLiteral("AnimateWithRotationOnly")); }

        WorldMode cameraMode() const { return _cameraMode; }
        void setCameraMode(WorldMode v) { setProperty(_cameraMode, v, QStringLiteral("CameraMode")); }

        int previewMaxTextureSize() const { return _previewMaxTextureSize; }
        void setPreviewMaxTextureSize(int v) { setProperty(_previewMaxTextureSize, v, QStringLiteral("PreviewMaxTextureSize")); }

        bool previewStaticMeshes() const { return _previewStaticMeshes; }
        void setPreviewStaticMeshes(bool v) { setProperty(_previewStaticMeshes, v, QStringLiteral("PreviewStaticMeshes")); }

        bool previewSkeletalMeshes() const { return _previewSkeletalMeshes; }
        void setPreviewSkeletalMeshes(bool v) { setProperty(_previewSkeletalMeshes, v, QStringLiteral("PreviewSkeletalMeshes")); }

        bool previewAnimations() const { return _previewAnimations; }
        void setPreviewAnimations(bool v) { setProperty(_previewAnimations, v, QStringLiteral("PreviewAnimations")); }

        bool previewMaterials() const { return _previewMaterials; }
        void setPreviewMaterials(bool v) { setProperty(_previewMaterials, v, QStringLiteral("PreviewMaterials")); }

        bool previewWorlds() const { return _previewWorlds; }
        void setPreviewWorlds(bool v) { setProperty(_previewWorlds, v, QStringLiteral("PreviewWorlds")); }

        bool saveMorphTargets() const { return _saveMorphTargets; }
        void setSaveMorphTargets(bool v) { setProperty(_saveMorphTargets, v, QStringLiteral("SaveMorphTargets")); }

        bool saveEmbeddedMaterials() const { return _saveEmbeddedMaterials; }
        void setSaveEmbeddedMaterials(bool v) { setProperty(_saveEmbeddedMaterials, v, QStringLiteral("SaveEmbeddedMaterials")); }

        bool saveSkeletonAsMesh() const { return _saveSkeletonAsMesh; }
        void setSaveSkeletonAsMesh(bool v) { setProperty(_saveSkeletonAsMesh, v, QStringLiteral("SaveSkeletonAsMesh")); }

        bool saveHdrTexturesAsHdr() const { return _saveHdrTexturesAsHdr; }
        void setSaveHdrTexturesAsHdr(bool v) { setProperty(_saveHdrTexturesAsHdr, v, QStringLiteral("SaveHdrTexturesAsHdr")); }

        bool featurePreviewNewAssetExplorer() const { return _featurePreviewNewAssetExplorer; }
        void setFeaturePreviewNewAssetExplorer(bool v) { setProperty(_featurePreviewNewAssetExplorer, v, QStringLiteral("FeaturePreviewNewAssetExplorer")); }

        bool previewTexturesAssetExplorer() const { return _previewTexturesAssetExplorer; }
        void setPreviewTexturesAssetExplorer(bool v) { setProperty(_previewTexturesAssetExplorer, v, QStringLiteral("PreviewTexturesAssetExplorer")); }

        // --- serialisation -----------------------------------------------------------------------------
        QJsonObject toJson() const;
        void readJson(const QJsonObject& json);

        // Named so that tests (and a future --settings switch) can round-trip without touching the real file.
        bool saveTo(const QString& filePath) const;
        static UserSettings* loadFrom(const QString& filePath, QObject* parent = nullptr);

    private:
        void clearPerDirectory();

        bool _showChangelog = true;
        QString _outputDirectory;
        QString _rawDataDirectory;
        QString _propertiesDirectory;
        QString _textureDirectory;
        QString _audioDirectory;
        QString _codeDirectory;
        QString _modelDirectory;
        QString _gameDirectory;
        int _lastOpenedSettingTab = 0;
        bool _isLoggerExpanded = true;
        GridLength _avalonImageSize{200.0};
        QString _audioDeviceId;
        float _audioPlayerVolume = 50.0F;
        ELoadingMode _loadingMode = ELoadingMode::All;
        QDateTime _lastUpdateCheck;                 // DateTime.MinValue
        QDateTime _nextUpdateCheck;                 // DateTime.Now, set in the constructor
        bool _keepDirectoryStructure = true;
        bool _showDecompileOption = false;
        ECompressedAudio _compressedAudioMode = ECompressedAudio::PlayDecompressed;
        EAesReload _aesReload = EAesReload::OncePerDay;
        EDiscordRpc _discordRpc = EDiscordRpc::Always;
        ELanguage _assetLanguage = ELanguage::English;
        EIconStyle _cosmeticStyle = EIconStyle::Default;
        bool _cosmeticDisplayAsset = false;
        int _imageMergerMargin = 5;
        bool _readScriptData = false;
        bool _readShaderMaps = false;
        bool _convertAudioOnBulkExport = false;
        bool _decompileLua = false;
        EUnluacFlags _unluacFlags = EUnluacFlags::Decompile;
        EJsonHighlightTheme _jsonHighlightTheme = EJsonHighlightTheme::Default;

        QHash<QString, DirectorySettings*> _perDirectory;
        DirectorySettings* _currentDir = nullptr;
        QHash<QString, DetectedGame> _manualGames;
        AuthResponse _lastAuthResponse;

        Hotkey* _dirLeftTab = nullptr;
        Hotkey* _dirRightTab = nullptr;
        Hotkey* _switchAssetExplorer = nullptr;
        Hotkey* _assetLeftTab = nullptr;
        Hotkey* _assetRightTab = nullptr;
        Hotkey* _assetAddTab = nullptr;
        Hotkey* _assetRemoveTab = nullptr;
        Hotkey* _addAudio = nullptr;
        Hotkey* _playPauseAudio = nullptr;
        Hotkey* _previousAudio = nullptr;
        Hotkey* _nextAudio = nullptr;

        EMeshFormat _meshExportFormat = EMeshFormat::UEFormat;
        ENaniteMeshFormat _naniteMeshExportFormat = ENaniteMeshFormat::OnlyNaniteLOD;
        EMaterialFormat _materialExportFormat = EMaterialFormat::FirstLayer;
        ETextureFormat _textureExportFormat = ETextureFormat::Png;
        ESocketFormat _socketExportFormat = ESocketFormat::Bone;
        EFileCompressionFormat _compressionFormat = EFileCompressionFormat::ZSTD;
        ELodFormat _lodExportFormat = ELodFormat::FirstLod;

        bool _showSkybox = true;
        bool _showGrid = true;
        bool _animateWithRotationOnly = false;
        WorldMode _cameraMode = WorldMode::Arcball;
        int _previewMaxTextureSize = 1024;
        bool _previewStaticMeshes = true;
        bool _previewSkeletalMeshes = true;
        bool _previewAnimations = true;
        bool _previewMaterials = true;
        bool _previewWorlds = true;
        bool _saveMorphTargets = true;
        bool _saveEmbeddedMaterials = true;
        bool _saveSkeletonAsMesh = false;
        bool _saveHdrTexturesAsHdr = true;
        bool _featurePreviewNewAssetExplorer = true;
        bool _previewTexturesAssetExplorer = true;
    };
}
