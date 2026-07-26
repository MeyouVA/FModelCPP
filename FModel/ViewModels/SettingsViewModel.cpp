// Ported from FModel/ViewModels/SettingsViewModel.cs
#include "SettingsViewModel.h"

#include "../Settings/DirectorySettings.h"
#include "../Settings/EndpointSettings.h"
#include "../Settings/UserSettings.h"

namespace FModel::ViewModels
{
    using Settings::DirectorySettings;
    using Settings::EndpointSettings;
    using Settings::UserSettings;

    void SettingsViewModel::setSelectedCustomVersions(const QList<FCustomVersion>& v)
    {
        _customVersionsReplaced = true;
        setProperty(_selectedCustomVersions, v, QStringLiteral("SelectedCustomVersions"));
    }

    void SettingsViewModel::setSelectedOptions(const QHash<QString, bool>& v)
    {
        _optionsReplaced = true;
        setProperty(_selectedOptions, v, QStringLiteral("SelectedOptions"));
    }

    void SettingsViewModel::setSelectedMapStructTypes(const QHash<QString, MapStructType>& v)
    {
        _mapStructTypesReplaced = true;
        setProperty(_selectedMapStructTypes, v, QStringLiteral("SelectedMapStructTypes"));
    }

    void SettingsViewModel::setAesEndpoint(EndpointSettings* v)
    {
        setProperty(_aesEndpoint, v, QStringLiteral("AesEndpoint"));
    }

    void SettingsViewModel::setMappingEndpoint(EndpointSettings* v)
    {
        setProperty(_mappingEndpoint, v, QStringLiteral("MappingEndpoint"));
    }

    void SettingsViewModel::setSelectedMeshExportFormat(EMeshFormat v)
    {
        // C# calls SetProperty and then raises the two derived properties unconditionally â€” outside the
        // `if changed` guard â€” so a re-selection of the same format still republishes them.
        setProperty(_selectedMeshExportFormat, v, QStringLiteral("SelectedMeshExportFormat"));
        raisePropertyChanged(QStringLiteral("SocketSettingsEnabled"));
        raisePropertyChanged(QStringLiteral("CompressionSettingsEnabled"));
    }

    void SettingsViewModel::initialize()
    {
        UserSettings* settings = UserSettings::Default();

        _outputSnapshot = settings->outputDirectory();
        _rawDataSnapshot = settings->rawDataDirectory();
        _propertiesSnapshot = settings->propertiesDirectory();
        _textureSnapshot = settings->textureDirectory();
        _audioSnapshot = settings->audioDirectory();
        _codeSnapshot = settings->codeDirectory();
        _modelSnapshot = settings->modelDirectory();
        _gameSnapshot = settings->gameDirectory();

        // C# dereferences CurrentDir unconditionally, because its ApplicationViewModel constructor exits the
        // process before this point when no game directory is configured. This port lets an unconfigured app
        // exist (see ApplicationViewModel.h), so the per-directory block is skipped when there is no CurrentDir
        // and the staged values keep their defaults.
        if (DirectorySettings* currentDir = settings->currentDir())
        {
            _uePlatformSnapshot = currentDir->texturePlatform();
            _ueGameSnapshot = currentDir->ueVersion();
            if (Settings::VersioningSettings* versioning = currentDir->versioning())
            {
                _selectedCustomVersions = versioning->customVersions();
                _selectedOptions = versioning->options();
                _selectedMapStructTypes = versioning->mapStructTypes();
            }
            _criwareDecryptionKey = currentDir->criwareDecryptionKey();
            _unluacOpcodeMap = currentDir->unluacOpCodeMap();

            const QList<EndpointSettings*>& endpoints = currentDir->endpoints();
            if (endpoints.size() > static_cast<int>(EEndpointType::Aes))
                setAesEndpoint(endpoints[static_cast<int>(EEndpointType::Aes)]);
            if (endpoints.size() > static_cast<int>(EEndpointType::Mapping))
                setMappingEndpoint(endpoints[static_cast<int>(EEndpointType::Mapping)]);
        }

        if (_mappingEndpoint != nullptr)
        {
            // C#: `MappingEndpoint.PropertyChanged += (_, args) => { if (!_mappingsUpdate) _mappingsUpdate =
            // args.PropertyName is "Overwrite" or "FilePath"; };` â€” note that once it is true nothing can
            // clear it, so undoing the edit before pressing OK still reloads the mappings.
            connect(_mappingEndpoint, &Framework::ViewModel::propertyChanged, this,
                    [this](const QString& propertyName)
                    {
                        if (!_mappingsUpdate)
                            _mappingsUpdate = propertyName == QStringLiteral("Overwrite") ||
                                              propertyName == QStringLiteral("FilePath");
                    });
        }

        _assetLanguageSnapshot = settings->assetLanguage();
        _compressedAudioSnapshot = settings->compressedAudioMode();
        _cosmeticStyleSnapshot = settings->cosmeticStyle();
        _meshExportFormatSnapshot = settings->meshExportFormat();
        _socketExportFormatSnapshot = settings->socketExportFormat();
        _compressionFormatSnapshot = settings->compressionFormat();
        _lodExportFormatSnapshot = settings->lodExportFormat();
        _naniteMeshExportFormatSnapshot = settings->naniteMeshExportFormat();
        _materialExportFormatSnapshot = settings->materialExportFormat();
        _textureExportFormatSnapshot = settings->textureExportFormat();
        _jsonHighlightThemeSnapshot = settings->jsonHighlightTheme();

        setSelectedUePlatform(_uePlatformSnapshot);
        setSelectedUeGame(_ueGameSnapshot);
        // The three versioning collections are assigned through the fields, not the setters: C# assigns the
        // very objects it snapshotted, which by reference equality means "not replaced".
        setSelectedAssetLanguage(_assetLanguageSnapshot);
        setSelectedCompressedAudio(_compressedAudioSnapshot);
        setSelectedCosmeticStyle(_cosmeticStyleSnapshot);
        setSelectedMeshExportFormat(_meshExportFormatSnapshot);
        setSelectedSocketExportFormat(_socketExportFormatSnapshot);
        // Upstream slip, kept: C# writes `SelectedCompressionFormat = _selectedCompressionFormat;` â€” the
        // backing field, still at its default â€” where every neighbouring line uses the snapshot. So the
        // compression combo opens on None rather than on the saved value, and pressing OK persists that.
        setSelectedCompressionFormat(_selectedCompressionFormat);
        setSelectedLodExportFormat(_lodExportFormatSnapshot);
        setSelectedNaniteMeshExportFormat(_naniteMeshExportFormatSnapshot);
        setSelectedMaterialExportFormat(_materialExportFormatSnapshot);
        setSelectedTextureExportFormat(_textureExportFormatSnapshot);
        setCriwareDecryptionKey(_criwareDecryptionKey);
        setUnluacOpcodeMap(_unluacOpcodeMap);
        setSelectedJsonHighlightTheme(_jsonHighlightThemeSnapshot);
        setSelectedAesReload(settings->aesReload());
        setSelectedDiscordRpc(settings->discordRpc());

        _ueGames = enumerateUeGames();
        _assetLanguages = {
            ELanguage::English, ELanguage::AustralianEnglish, ELanguage::BritishEnglish, ELanguage::French,
            ELanguage::German, ELanguage::Italian, ELanguage::Spanish, ELanguage::SpanishLatin,
            ELanguage::SpanishMexico, ELanguage::Arabic, ELanguage::Japanese, ELanguage::Korean,
            ELanguage::Polish, ELanguage::Portuguese, ELanguage::PortugueseBrazil, ELanguage::Russian,
            ELanguage::Turkish, ELanguage::Chinese, ELanguage::TraditionalChinese, ELanguage::Swedish,
            ELanguage::Thai, ELanguage::Indonesian, ELanguage::VietnameseVietnam, ELanguage::Zulu};
        _aesReloads = {EAesReload::Always, EAesReload::Never, EAesReload::OncePerDay};
        _discordRpcs = {EDiscordRpc::Always, EDiscordRpc::Never};
        _compressedAudios = {ECompressedAudio::PlayDecompressed, ECompressedAudio::PlayCompressed};
        _cosmeticStyles = {EIconStyle::Default, EIconStyle::NoBackground, EIconStyle::NoText,
                           EIconStyle::Flat, EIconStyle::Cataba};
        _meshExportFormats = {EMeshFormat::ActorX, EMeshFormat::Gltf2, EMeshFormat::OBJ, EMeshFormat::UEFormat};
        _socketExportFormats = {ESocketFormat::Socket, ESocketFormat::Bone, ESocketFormat::None};
        _compressionFormats = {EFileCompressionFormat::None, EFileCompressionFormat::GZIP,
                               EFileCompressionFormat::ZSTD};
        _lodExportFormats = {ELodFormat::FirstLod, ELodFormat::AllLods};
        _naniteMeshExportFormats = {ENaniteMeshFormat::OnlyNaniteLOD, ENaniteMeshFormat::OnlyNormalLODs,
                                    ENaniteMeshFormat::AllLayersNaniteFirst,
                                    ENaniteMeshFormat::AllLayersNaniteLast};
        _materialExportFormats = {EMaterialFormat::FirstLayer, EMaterialFormat::AllLayersNoRef,
                                  EMaterialFormat::AllLayers};
        _textureExportFormats = {ETextureFormat::Png, ETextureFormat::Jpeg, ETextureFormat::Tga,
                                 ETextureFormat::Dds};
        _platforms = {ETexturePlatform::DesktopMobile, ETexturePlatform::XboxAndPlaystation4,
                      ETexturePlatform::NintendoSwitch, ETexturePlatform::Playstation5};
        _jsonHighlightThemes = {
            EJsonHighlightTheme::Default, EJsonHighlightTheme::MintLavender, EJsonHighlightTheme::SoftBlueGreen,
            EJsonHighlightTheme::PurpleCyan, EJsonHighlightTheme::NeutralWarm, EJsonHighlightTheme::Nord,
            EJsonHighlightTheme::Mocha, EJsonHighlightTheme::TokyoNight, EJsonHighlightTheme::OneDark,
            EJsonHighlightTheme::GruvboxDark, EJsonHighlightTheme::RosePine, EJsonHighlightTheme::Monokai,
            EJsonHighlightTheme::Oceanic, EJsonHighlightTheme::Forest, EJsonHighlightTheme::Amber,
            EJsonHighlightTheme::Iceberg};
    }

    bool SettingsViewModel::save(QList<SettingsOut>& whatShouldIDo)
    {
        bool restart = false;
        whatShouldIDo.clear();

        UserSettings* settings = UserSettings::Default();

        if (_assetLanguageSnapshot != _selectedAssetLanguage)
            whatShouldIDo.append(SettingsOut::ReloadLocres);
        if (_mappingsUpdate)
            whatShouldIDo.append(SettingsOut::ReloadMappings);

        if (_ueGameSnapshot != _selectedUeGame || _customVersionsReplaced ||
            _uePlatformSnapshot != _selectedUePlatform || _optionsReplaced || // combobox
            _mapStructTypesReplaced ||
            _gameSnapshot != settings->gameDirectory()) // textbox
            restart = true;

        if (DirectorySettings* currentDir = settings->currentDir())
        {
            currentDir->setUeVersion(_selectedUeGame);
            currentDir->setTexturePlatform(_selectedUePlatform);
            if (Settings::VersioningSettings* versioning = currentDir->versioning())
            {
                versioning->setCustomVersions(_selectedCustomVersions);
                versioning->setOptions(_selectedOptions);
                versioning->setMapStructTypes(_selectedMapStructTypes);
            }
            currentDir->setCriwareDecryptionKey(_criwareDecryptionKey);
            currentDir->setUnluacOpCodeMap(_unluacOpcodeMap);
        }

        settings->setAssetLanguage(_selectedAssetLanguage);
        settings->setCompressedAudioMode(_selectedCompressedAudio);
        settings->setCosmeticStyle(_selectedCosmeticStyle);
        settings->setMeshExportFormat(_selectedMeshExportFormat);
        settings->setSocketExportFormat(_selectedSocketExportFormat);
        settings->setCompressionFormat(_selectedCompressionFormat);
        settings->setLodExportFormat(_selectedLodExportFormat);
        settings->setNaniteMeshExportFormat(_selectedNaniteMeshExportFormat);
        settings->setMaterialExportFormat(_selectedMaterialExportFormat);
        settings->setTextureExportFormat(_selectedTextureExportFormat);
        settings->setAesReload(_selectedAesReload);
        settings->setDiscordRpc(_selectedDiscordRpc);
        settings->setJsonHighlightTheme(_selectedJsonHighlightTheme);

        // TODO: C# also runs `if (SelectedDiscordRpc == EDiscordRpc.Never) _discordHandler.Shutdown();` here.
        // DiscordService/DiscordHandler are not ported, so turning the presence off only persists the setting.

        return restart;
    }

    QList<SettingsViewModel::EGame> SettingsViewModel::enumerateUeGames()
    {
        // C# declares this method twice, character for character: once here and once on
        // GameSelectorViewModel. Rather than duplicate it, this one delegates; the implementation (and its
        // explanation of the GroupBy/OrderBy pipeline) lives there because it sits lower in the build's
        // layering. Behaviour is identical, which is the point.
        return GameSelectorViewModel::enumerateUeGames();
    }
}
