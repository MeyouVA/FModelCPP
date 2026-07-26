#pragma once
// Ported from FModel/ViewModels/SettingsViewModel.cs — the view-model behind the Settings window.
//
// It is a staging area, not storage: Initialize() snapshots UserSettings, the window edits the Selected*
// properties, and Save() writes them back and reports what the app has to redo. Everything the window binds
// straight to UserSettings.Default (the directories, the toggles, the hotkeys) never passes through here.
//
// Deliberate differences from C#:
//   * C# holds the three versioning collections as interfaces (IList / IDictionary) and Save() compares them
//     to their snapshots with `!=`, i.e. by REFERENCE. Initialize() assigns the very object it snapshotted, so
//     that test is false until the dictionary editor hands the view-model a freshly deserialized object — the
//     real meaning is "was the editor OK'd", and OK'ing it with the JSON untouched still forces a restart. The
//     C++ members are values, where `!=` would compare contents and silently drop that quirk, so each carries
//     a *Replaced flag set by its public setter (the editor's path) and left alone by Initialize().
//   * ReadOnlyObservableCollection<T> becomes a plain QList<T>: the lists are built once in Initialize() and
//     never mutated, so the collection-changed half of the type has nothing to report.
//   * The Save() arm that shuts the Discord RPC handler down is deferred — DiscordService/DiscordHandler are
//     not ported. See the TODO at the call site.
//   * Enum.GetValues<T>() has no C++ equivalent; each Enumerate*() spells its enum's members out instead, in
//     declaration order (which is what .NET yields for these). EGame is the exception: 231 members, listed
//     once in CUE4Parse's EGameValues() so they cannot drift from EGameName's switch.

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>

#include "../Enums.h"
#include "../Extensions/Themes/JsonHighlightThemes.h"
#include "../Framework/ViewModel.h"
#include "../Settings/VersioningSettings.h"

#include "UE4/Assets/Exports/Material/EMaterialFormat.h"
#include "UE4/Assets/Exports/Nanite/ENaniteMeshFormat.h"
#include "UE4/Assets/Exports/Texture/ETexturePlatform.h"
#include "UE4/Objects/Core/Serialization/FCustomVersion.h"
#include "UE4/Versions/EGame.h"
#include "UE4/Versions/ELanguage.h"

#include "Meshes/ELodFormat.h"
#include "Meshes/EMeshFormat.h"
#include "Meshes/ESocketFormat.h"
#include "Textures/ETextureFormat.h"
#include "UEFormat/Enums/EFileCompressionFormat.h"

namespace FModel::Settings { class EndpointSettings; }

namespace FModel::ViewModels
{
    class SettingsViewModel : public Framework::ViewModel
    {
        Q_OBJECT

    public:
        using EGame = CUE4Parse::UE4::Versions::EGame;
        using ELanguage = CUE4Parse::UE4::Versions::ELanguage;
        using ETexturePlatform = CUE4Parse::UE4::Assets::Exports::Texture::ETexturePlatform;
        using EMaterialFormat = CUE4Parse::UE4::Assets::Exports::Material::EMaterialFormat;
        using ENaniteMeshFormat = CUE4Parse::UE4::Assets::Exports::Nanite::ENaniteMeshFormat;
        using ELodFormat = CUE4Parse_Conversion::Meshes::ELodFormat;
        using EMeshFormat = CUE4Parse_Conversion::Meshes::EMeshFormat;
        using ESocketFormat = CUE4Parse_Conversion::Meshes::ESocketFormat;
        using ETextureFormat = CUE4Parse_Conversion::Textures::ETextureFormat;
        using EFileCompressionFormat = CUE4Parse_Conversion::UEFormat::Enums::EFileCompressionFormat;
        using EJsonHighlightTheme = Extensions::Themes::EJsonHighlightTheme;
        using FCustomVersion = CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersion;
        using MapStructType = Settings::MapStructType;

        explicit SettingsViewModel(QObject* parent = nullptr) : ViewModel(parent) {}

        // Snapshots UserSettings and fills the combo-box sources. Called by the window's constructor.
        void initialize();

        // Writes the staged values back into UserSettings, appends to `whatShouldIDo` the reloads the app owes
        // as a result, and returns whether a restart is needed. Mirrors C#'s `bool Save(out List<SettingsOut>)`.
        bool save(QList<SettingsOut>& whatShouldIDo);

        // --- staged values -----------------------------------------------------------------------------
        bool useCustomOutputFolders() const { return _useCustomOutputFolders; }
        void setUseCustomOutputFolders(bool v)
        { setProperty(_useCustomOutputFolders, v, QStringLiteral("UseCustomOutputFolders")); }

        ETexturePlatform selectedUePlatform() const { return _selectedUePlatform; }
        void setSelectedUePlatform(ETexturePlatform v)
        { setProperty(_selectedUePlatform, v, QStringLiteral("SelectedUePlatform")); }

        EGame selectedUeGame() const { return _selectedUeGame; }
        void setSelectedUeGame(EGame v) { setProperty(_selectedUeGame, v, QStringLiteral("SelectedUeGame")); }

        // The three setters below are the dictionary editor's path, so each also raises its *Replaced flag —
        // see the reference-vs-value note at the top of this file.
        const QList<FCustomVersion>& selectedCustomVersions() const { return _selectedCustomVersions; }
        void setSelectedCustomVersions(const QList<FCustomVersion>& v);

        const QHash<QString, bool>& selectedOptions() const { return _selectedOptions; }
        void setSelectedOptions(const QHash<QString, bool>& v);

        const QHash<QString, MapStructType>& selectedMapStructTypes() const { return _selectedMapStructTypes; }
        void setSelectedMapStructTypes(const QHash<QString, MapStructType>& v);

        // Not owned: both point into UserSettings.Default.CurrentDir.Endpoints.
        Settings::EndpointSettings* aesEndpoint() const { return _aesEndpoint; }
        void setAesEndpoint(Settings::EndpointSettings* v);
        Settings::EndpointSettings* mappingEndpoint() const { return _mappingEndpoint; }
        void setMappingEndpoint(Settings::EndpointSettings* v);

        ELanguage selectedAssetLanguage() const { return _selectedAssetLanguage; }
        void setSelectedAssetLanguage(ELanguage v)
        { setProperty(_selectedAssetLanguage, v, QStringLiteral("SelectedAssetLanguage")); }

        EAesReload selectedAesReload() const { return _selectedAesReload; }
        void setSelectedAesReload(EAesReload v)
        { setProperty(_selectedAesReload, v, QStringLiteral("SelectedAesReload")); }

        EDiscordRpc selectedDiscordRpc() const { return _selectedDiscordRpc; }
        void setSelectedDiscordRpc(EDiscordRpc v)
        { setProperty(_selectedDiscordRpc, v, QStringLiteral("SelectedDiscordRpc")); }

        ECompressedAudio selectedCompressedAudio() const { return _selectedCompressedAudio; }
        void setSelectedCompressedAudio(ECompressedAudio v)
        { setProperty(_selectedCompressedAudio, v, QStringLiteral("SelectedCompressedAudio")); }

        EIconStyle selectedCosmeticStyle() const { return _selectedCosmeticStyle; }
        void setSelectedCosmeticStyle(EIconStyle v)
        { setProperty(_selectedCosmeticStyle, v, QStringLiteral("SelectedCosmeticStyle")); }

        // Also republishes the two derived flags below, exactly as the C# setter does.
        EMeshFormat selectedMeshExportFormat() const { return _selectedMeshExportFormat; }
        void setSelectedMeshExportFormat(EMeshFormat v);

        ESocketFormat selectedSocketExportFormat() const { return _selectedSocketExportFormat; }
        void setSelectedSocketExportFormat(ESocketFormat v)
        { setProperty(_selectedSocketExportFormat, v, QStringLiteral("SelectedSocketExportFormat")); }

        EFileCompressionFormat selectedCompressionFormat() const { return _selectedCompressionFormat; }
        void setSelectedCompressionFormat(EFileCompressionFormat v)
        { setProperty(_selectedCompressionFormat, v, QStringLiteral("SelectedCompressionFormat")); }

        ELodFormat selectedLodExportFormat() const { return _selectedLodExportFormat; }
        void setSelectedLodExportFormat(ELodFormat v)
        { setProperty(_selectedLodExportFormat, v, QStringLiteral("SelectedLodExportFormat")); }

        ENaniteMeshFormat selectedNaniteMeshExportFormat() const { return _selectedNaniteMeshExportFormat; }
        void setSelectedNaniteMeshExportFormat(ENaniteMeshFormat v)
        { setProperty(_selectedNaniteMeshExportFormat, v, QStringLiteral("SelectedNaniteMeshExportFormat")); }

        EMaterialFormat selectedMaterialExportFormat() const { return _selectedMaterialExportFormat; }
        void setSelectedMaterialExportFormat(EMaterialFormat v)
        { setProperty(_selectedMaterialExportFormat, v, QStringLiteral("SelectedMaterialExportFormat")); }

        ETextureFormat selectedTextureExportFormat() const { return _selectedTextureExportFormat; }
        void setSelectedTextureExportFormat(ETextureFormat v)
        { setProperty(_selectedTextureExportFormat, v, QStringLiteral("SelectedTextureExportFormat")); }

        EJsonHighlightTheme selectedJsonHighlightTheme() const { return _selectedJsonHighlightTheme; }
        void setSelectedJsonHighlightTheme(EJsonHighlightTheme v)
        { setProperty(_selectedJsonHighlightTheme, v, QStringLiteral("SelectedJsonHighlightTheme")); }

        quint64 criwareDecryptionKey() const { return _criwareDecryptionKey; }
        void setCriwareDecryptionKey(quint64 v)
        { setProperty(_criwareDecryptionKey, v, QStringLiteral("CriwareDecryptionKey")); }

        const QString& unluacOpcodeMap() const { return _unluacOpcodeMap; }
        void setUnluacOpcodeMap(const QString& v)
        { setProperty(_unluacOpcodeMap, v, QStringLiteral("UnluacOpcodeMap")); }

        // The two format-dependent enables the Models page greys its rows with.
        bool socketSettingsEnabled() const { return _selectedMeshExportFormat == EMeshFormat::ActorX; }
        bool compressionSettingsEnabled() const { return _selectedMeshExportFormat == EMeshFormat::UEFormat; }

        // --- combo-box sources (built by initialize(), never mutated afterwards) ------------------------
        const QList<EGame>& ueGames() const { return _ueGames; }
        const QList<ELanguage>& assetLanguages() const { return _assetLanguages; }
        const QList<EAesReload>& aesReloads() const { return _aesReloads; }
        const QList<EDiscordRpc>& discordRpcs() const { return _discordRpcs; }
        const QList<ECompressedAudio>& compressedAudios() const { return _compressedAudios; }
        const QList<EIconStyle>& cosmeticStyles() const { return _cosmeticStyles; }
        const QList<EMeshFormat>& meshExportFormats() const { return _meshExportFormats; }
        const QList<ESocketFormat>& socketExportFormats() const { return _socketExportFormats; }
        const QList<EFileCompressionFormat>& compressionFormats() const { return _compressionFormats; }
        const QList<ELodFormat>& lodExportFormats() const { return _lodExportFormats; }
        const QList<ENaniteMeshFormat>& naniteMeshExportFormats() const { return _naniteMeshExportFormats; }
        const QList<EMaterialFormat>& materialExportFormats() const { return _materialExportFormats; }
        const QList<ETextureFormat>& textureExportFormats() const { return _textureExportFormats; }
        const QList<ETexturePlatform>& platforms() const { return _platforms; }
        const QList<EJsonHighlightTheme>& jsonHighlightThemes() const { return _jsonHighlightThemes; }

        // C#'s private EnumerateUeGames(): every EGame value, deduplicated, then stably reordered so the
        // game-specific members come before the base engine versions.
        static QList<EGame> enumerateUeGames();

    private:
        bool _useCustomOutputFolders = false;
        ETexturePlatform _selectedUePlatform = ETexturePlatform::DesktopMobile;
        EGame _selectedUeGame = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        QList<FCustomVersion> _selectedCustomVersions;
        QHash<QString, bool> _selectedOptions;
        QHash<QString, MapStructType> _selectedMapStructTypes;
        Settings::EndpointSettings* _aesEndpoint = nullptr;
        Settings::EndpointSettings* _mappingEndpoint = nullptr;
        ELanguage _selectedAssetLanguage = ELanguage::English;
        EAesReload _selectedAesReload = EAesReload::Never;
        EDiscordRpc _selectedDiscordRpc = EDiscordRpc::Always;
        ECompressedAudio _selectedCompressedAudio = ECompressedAudio::PlayDecompressed;
        EIconStyle _selectedCosmeticStyle = EIconStyle::Default;
        EMeshFormat _selectedMeshExportFormat = EMeshFormat::ActorX;
        ESocketFormat _selectedSocketExportFormat = ESocketFormat::Socket;
        EFileCompressionFormat _selectedCompressionFormat = EFileCompressionFormat::None;
        ELodFormat _selectedLodExportFormat = ELodFormat::FirstLod;
        ENaniteMeshFormat _selectedNaniteMeshExportFormat = ENaniteMeshFormat::OnlyNaniteLOD;
        EMaterialFormat _selectedMaterialExportFormat = EMaterialFormat::FirstLayer;
        ETextureFormat _selectedTextureExportFormat = ETextureFormat::Png;
        EJsonHighlightTheme _selectedJsonHighlightTheme = EJsonHighlightTheme::Default;
        quint64 _criwareDecryptionKey = 0;
        QString _unluacOpcodeMap;

        QList<EGame> _ueGames;
        QList<ELanguage> _assetLanguages;
        QList<EAesReload> _aesReloads;
        QList<EDiscordRpc> _discordRpcs;
        QList<ECompressedAudio> _compressedAudios;
        QList<EIconStyle> _cosmeticStyles;
        QList<EMeshFormat> _meshExportFormats;
        QList<ESocketFormat> _socketExportFormats;
        QList<EFileCompressionFormat> _compressionFormats;
        QList<ELodFormat> _lodExportFormats;
        QList<ENaniteMeshFormat> _naniteMeshExportFormats;
        QList<EMaterialFormat> _materialExportFormats;
        QList<ETextureFormat> _textureExportFormats;
        QList<ETexturePlatform> _platforms;
        QList<EJsonHighlightTheme> _jsonHighlightThemes;

        // The snapshots taken by initialize(). Seven of them (_outputSnapshot .. _modelSnapshot) and six of
        // the format snapshots are never read again — C# captures them too, and dropping them here would hide
        // that the restart test deliberately ignores the output directories.
        QString _outputSnapshot;
        QString _rawDataSnapshot;
        QString _propertiesSnapshot;
        QString _textureSnapshot;
        QString _audioSnapshot;
        QString _codeSnapshot;
        QString _modelSnapshot;
        QString _gameSnapshot;
        ETexturePlatform _uePlatformSnapshot = ETexturePlatform::DesktopMobile;
        EGame _ueGameSnapshot = CUE4Parse::UE4::Versions::GAME_UE4_LATEST;
        ELanguage _assetLanguageSnapshot = ELanguage::English;
        ECompressedAudio _compressedAudioSnapshot = ECompressedAudio::PlayDecompressed;
        EIconStyle _cosmeticStyleSnapshot = EIconStyle::Default;
        EMeshFormat _meshExportFormatSnapshot = EMeshFormat::ActorX;
        ESocketFormat _socketExportFormatSnapshot = ESocketFormat::Socket;
        EFileCompressionFormat _compressionFormatSnapshot = EFileCompressionFormat::None;
        ELodFormat _lodExportFormatSnapshot = ELodFormat::FirstLod;
        ENaniteMeshFormat _naniteMeshExportFormatSnapshot = ENaniteMeshFormat::OnlyNaniteLOD;
        EMaterialFormat _materialExportFormatSnapshot = EMaterialFormat::FirstLayer;
        ETextureFormat _textureExportFormatSnapshot = ETextureFormat::Png;
        EJsonHighlightTheme _jsonHighlightThemeSnapshot = EJsonHighlightTheme::Default;

        // C#'s reference comparisons against _customVersionsSnapshot / _optionsSnapshot /
        // _mapStructTypesSnapshot, which are only ever unequal once the dictionary editor replaces the object.
        bool _customVersionsReplaced = false;
        bool _optionsReplaced = false;
        bool _mapStructTypesReplaced = false;

        bool _mappingsUpdate = false;
    };
}
