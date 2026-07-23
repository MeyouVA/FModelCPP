// Behavioural tests for the ported Settings layer (Settings/*, Framework/Hotkey, Framework/GridLength).
//
// The point of most of these is *format* compatibility: AppSettings.json is written by the C# FModel that
// people already have installed, so the port has to read and rewrite it without losing or reinterpreting
// anything. The fixture below is a hand-built document with the same shape as a real settings file — real
// files are deliberately not read here, since they contain live AES keys and the suite must be reproducible
// on a machine that has never run FModel.

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "Framework/GridLength.h"
#include "Framework/Hotkey.h"
#include "Settings/CustomDirectory.h"
#include "Settings/DirectorySettings.h"
#include "Settings/EndpointSettings.h"
#include "Settings/UserSettings.h"
#include "Settings/VersioningSettings.h"

using namespace FModel::Framework;
using namespace FModel::Settings;
using FModel::EAesReload;
using FModel::ECompressedAudio;
using FModel::EEndpointType;
using FModel::EIconStyle;
using FModel::ELoadingMode;
using FModel::EUnluacMode;
using CUE4Parse::UE4::Lua::unluac::EUnluacFlags;

class TestSettings : public QObject
{
    Q_OBJECT

private slots:
    // ---------------------------------------------------------------- Framework/InputKeys + Hotkey

    // These numbers are not ours to choose: they are System.Windows.Input.Key's values, and they are what sits
    // in every existing AppSettings.json. The anchors below are read straight out of real settings files.
    void keyValuesMatchWpf()
    {
        QCOMPARE(static_cast<int>(Key::None), 0);
        QCOMPARE(static_cast<int>(Key::A), 44);
        QCOMPARE(static_cast<int>(Key::D), 47);
        QCOMPARE(static_cast<int>(Key::E), 48);
        QCOMPARE(static_cast<int>(Key::J), 53);
        QCOMPARE(static_cast<int>(Key::K), 54);
        QCOMPARE(static_cast<int>(Key::L), 55);
        QCOMPARE(static_cast<int>(Key::N), 57);
        QCOMPARE(static_cast<int>(Key::Q), 60);
        QCOMPARE(static_cast<int>(Key::T), 63);
        QCOMPARE(static_cast<int>(Key::W), 66);
        QCOMPARE(static_cast<int>(Key::Z), 69);
        QCOMPARE(static_cast<int>(Key::D0), 34);
        QCOMPARE(static_cast<int>(Key::F1), 90);
        QCOMPARE(static_cast<int>(Key::F12), 101);

        QCOMPARE(static_cast<int>(ModifierKeys::None), 0);
        QCOMPARE(static_cast<int>(ModifierKeys::Alt), 1);
        QCOMPARE(static_cast<int>(ModifierKeys::Control), 2);
        QCOMPARE(static_cast<int>(ModifierKeys::Shift), 4);
        QCOMPARE(static_cast<int>(ModifierKeys::Windows), 8);

        // Aliases must not introduce new values.
        QCOMPARE(static_cast<int>(Key::Enter), static_cast<int>(Key::Return));
        QCOMPARE(static_cast<int>(Key::PageUp), static_cast<int>(Key::Prior));

        // keyName stands in for C#'s Key.ToString().
        QCOMPARE(keyName(Key::A), QStringLiteral("A"));
        QCOMPARE(keyName(Key::F12), QStringLiteral("F12"));
        QCOMPARE(keyName(Key::OemComma), QStringLiteral("OemComma"));
    }

    void hotkeyToStringMatchesCSharp()
    {
        QCOMPARE(Hotkey(Key::A).toString(), QStringLiteral("A"));
        QCOMPARE(Hotkey(Key::T, ModifierKeys::Control).toString(), QStringLiteral("Ctrl + T"));
        QCOMPARE(Hotkey(Key::W, ModifierKeys::Control | ModifierKeys::Shift).toString(),
                 QStringLiteral("Ctrl + Shift + W"));
        QCOMPARE(Hotkey(Key::F1, ModifierKeys::Alt | ModifierKeys::Windows).toString(),
                 QStringLiteral("Alt + Win + F1"));
    }

    void hotkeyIsTriggered()
    {
        const Hotkey plain(Key::K);
        QVERIFY(plain.isTriggered(Key::K, ModifierKeys::None));
        QVERIFY(!plain.isTriggered(Key::J, ModifierKeys::None));
        // C#'s HasFlag(None) is always true, so an unmodified hotkey still fires while Ctrl is held.
        QVERIFY(plain.isTriggered(Key::K, ModifierKeys::Control));

        const Hotkey combo(Key::T, ModifierKeys::Control);
        QVERIFY(combo.isTriggered(Key::T, ModifierKeys::Control));
        QVERIFY(combo.isTriggered(Key::T, ModifierKeys::Control | ModifierKeys::Shift));
        QVERIFY(!combo.isTriggered(Key::T, ModifierKeys::None));
    }

    void hotkeyRoundTripsThroughJson()
    {
        const Hotkey source(Key::T, ModifierKeys::Control);
        const QJsonObject json = source.toJson();
        QCOMPARE(json[QStringLiteral("Key")].toInt(), 63);
        QCOMPARE(json[QStringLiteral("Modifiers")].toInt(), 2);

        Hotkey target(Key::None);
        target.readJson(json);
        QCOMPARE(target.key(), Key::T);
        QCOMPARE(target.modifiers(), ModifierKeys::Control);
    }

    // ---------------------------------------------------------------- Framework/GridLength

    void gridLengthMatchesWpfConverter()
    {
        // The one form that actually appears in settings files is an absolute pixel count, stored as a string.
        QCOMPARE(GridLength(730.0).toString(), QStringLiteral("730"));
        QCOMPARE(GridLength().toString(), QStringLiteral("Auto"));
        QCOMPARE(GridLength(1.0, GridUnitType::Star).toString(), QStringLiteral("*"));
        QCOMPARE(GridLength(2.0, GridUnitType::Star).toString(), QStringLiteral("2*"));

        QVERIFY(GridLength::fromString(QStringLiteral("730")).isAbsolute());
        QCOMPARE(GridLength::fromString(QStringLiteral("730")).value(), 730.0);
        QVERIFY(GridLength::fromString(QStringLiteral("Auto")).isAuto());
        QVERIFY(GridLength::fromString(QStringLiteral("*")).isStar());
        QCOMPARE(GridLength::fromString(QStringLiteral("2*")).value(), 2.0);

        // Garbage degrades to Auto rather than throwing, matching a default-constructed GridLength.
        QVERIFY(GridLength::fromString(QStringLiteral("not a length")).isAuto());

        QCOMPARE(GridLength::fromString(GridLength(200.0).toString()), GridLength(200.0));
    }

    // Opt-in check against a settings file written by the real C# app. Skipped unless
    // FMODEL_SETTINGS_FIXTURE points at one, because such a file contains live AES keys and the suite has to
    // stay hermetic and reproducible. Nothing here prints or asserts on key material — only on structure.
    void realSettingsFileRoundTrips()
    {
        const QByteArray path = qgetenv("FMODEL_SETTINGS_FIXTURE");
        if (path.isEmpty())
            QSKIP("set FMODEL_SETTINGS_FIXTURE to a real AppSettings.json to run this");

        QFile file(QString::fromLocal8Bit(path));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        QVERIFY2(parseError.error == QJsonParseError::NoError, qPrintable(parseError.errorString()));
        QVERIFY(document.isObject());

        const QJsonObject original = document.object();
        UserSettings settings;
        settings.readJson(original);

        // Every key the real writer emitted must survive a read/write cycle...
        const QJsonObject rewritten = settings.toJson();
        for (auto it = original.constBegin(); it != original.constEnd(); ++it)
            QVERIFY2(rewritten.contains(it.key()), qPrintable(it.key()));

        // ...and the second pass must be a fixed point, which is the real test: it proves nothing was
        // silently dropped, defaulted, or re-typed on the way through.
        UserSettings second;
        second.readJson(rewritten);
        QCOMPARE(second.toJson(), rewritten);

        // Sanity: the per-directory blocks actually parsed rather than quietly coming back empty.
        QCOMPARE(settings.perDirectory().size(), original[QStringLiteral("PerDirectory")].toObject().size());
        for (DirectorySettings* directory : settings.perDirectory())
        {
            QVERIFY(!directory->gameDirectory().isEmpty());
            QCOMPARE(directory->endpoints().size(), 2);
        }
    }

    // ---------------------------------------------------------------- Settings/CustomDirectory

    void customDirectoryDefaults()
    {
        const QList<CustomDirectory*> fortnite = CustomDirectory::Default(QStringLiteral("Fortnite"));
        QCOMPARE(fortnite.size(), 5);
        QCOMPARE(fortnite.first()->header(), QStringLiteral("Cosmetics"));
        QCOMPARE(CustomDirectory::Default(QStringLiteral("Fortnite [LIVE]")).size(), 5);
        QCOMPARE(CustomDirectory::Default(QStringLiteral("VALORANT")).size(), 6);
        QCOMPARE(CustomDirectory::Default(QStringLiteral("Dead by Daylight")).size(), 11);
        QCOMPARE(CustomDirectory::Default(QStringLiteral("Some Other Game")).size(), 0);
        qDeleteAll(fortnite);
    }

    // ---------------------------------------------------------------- Settings/EndpointSettings

    void endpointDefaultsAndValidity()
    {
        const QList<EndpointSettings*> fortnite = EndpointSettings::Default(QStringLiteral("Fortnite"));
        QCOMPARE(fortnite.size(), 2);

        // The array is indexed by EEndpointType, so order is part of the contract.
        EndpointSettings* aes = fortnite.at(static_cast<int>(EEndpointType::Aes));
        EndpointSettings* mappings = fortnite.at(static_cast<int>(EEndpointType::Mapping));
        QVERIFY(aes->url().endsWith(QStringLiteral("/aes")));
        QVERIFY(mappings->url().endsWith(QStringLiteral("/mappings")));
        QVERIFY(aes->isValid());
        QVERIFY(aes->label().startsWith(QStringLiteral("Your endpoint configuration is valid")));

        const QList<EndpointSettings*> unknown = EndpointSettings::Default(QStringLiteral("Some Other Game"));
        QCOMPARE(unknown.size(), 2);
        QVERIFY(!unknown.at(0)->isValid());
        QVERIFY(unknown.at(0)->label().contains(QStringLiteral("DOES NOT seem to be valid")));

        qDeleteAll(fortnite);
        qDeleteAll(unknown);
    }

    // Setting IsValid refreshes the derived Label even when the value did not actually change.
    void endpointIsValidAlwaysRefreshesLabel()
    {
        EndpointSettings endpoint;
        QSignalSpy spy(&endpoint, &FModel::Framework::ViewModel::propertyChanged);

        endpoint.setIsValid(false); // no change to IsValid...
        QCOMPARE(spy.count(), 1);   // ...but Label still fires
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("Label"));

        endpoint.setIsValid(true);
        QCOMPARE(spy.count(), 2);   // IsValid + Label
    }

    // ---------------------------------------------------------------- Settings/UserSettings

    void userSettingsDefaultsMatchCSharp()
    {
        UserSettings settings;
        QCOMPARE(settings.showChangelog(), true);
        QCOMPARE(settings.isLoggerExpanded(), true);
        QCOMPARE(settings.avalonImageSize(), GridLength(200.0));
        QCOMPARE(settings.audioPlayerVolume(), 50.0F);
        QCOMPARE(settings.loadingMode(), ELoadingMode::All);
        QCOMPARE(settings.keepDirectoryStructure(), true);
        QCOMPARE(settings.showDecompileOption(), false);
        QCOMPARE(settings.compressedAudioMode(), ECompressedAudio::PlayDecompressed);
        QCOMPARE(settings.aesReload(), EAesReload::OncePerDay);
        QCOMPARE(settings.cosmeticStyle(), EIconStyle::Default);
        QCOMPARE(settings.imageMergerMargin(), 5);
        QCOMPARE(settings.previewMaxTextureSize(), 1024);
        QCOMPARE(settings.saveHdrTexturesAsHdr(), true);
        QCOMPARE(settings.saveSkeletonAsMesh(), false);
        QCOMPARE(settings.unluacFlags(), EUnluacFlags::Decompile);

        // Default hotkeys.
        QCOMPARE(settings.dirLeftTab()->key(), Key::A);
        QCOMPARE(settings.dirRightTab()->key(), Key::D);
        QCOMPARE(settings.switchAssetExplorer()->key(), Key::Z);
        QCOMPARE(settings.assetAddTab()->key(), Key::T);
        QCOMPARE(settings.assetAddTab()->modifiers(), ModifierKeys::Control);
        QCOMPARE(settings.nextAudio()->key(), Key::L);
    }

    // UnluacMode is a projection of UnluacFlags: setting it must preserve the unrelated flag bits.
    void unluacModeProjectsOntoFlags()
    {
        UserSettings settings;
        QCOMPARE(settings.unluacMode(), EUnluacMode::Decompile);

        settings.setUnluacFlags(EUnluacFlags::Decompile | EUnluacFlags::NoDebug | EUnluacFlags::OpCodeMap);
        QCOMPARE(settings.unluacMode(), EUnluacMode::Decompile);

        QSignalSpy spy(&settings, &FModel::Framework::ViewModel::propertyChanged);
        settings.setUnluacMode(EUnluacMode::Disassemble);
        QCOMPARE(settings.unluacMode(), EUnluacMode::Disassemble);

        // NoDebug and OpCodeMap survive; Decompile is swapped out for Disassemble.
        QVERIFY(HasFlag(settings.unluacFlags(), EUnluacFlags::NoDebug));
        QVERIFY(HasFlag(settings.unluacFlags(), EUnluacFlags::OpCodeMap));
        QVERIFY(HasFlag(settings.unluacFlags(), EUnluacFlags::Disassemble));
        QVERIFY(!HasFlag(settings.unluacFlags(), EUnluacFlags::Decompile));

        // Changing the flags raises UnluacMode as well, so a bound UI updates.
        QStringList names;
        for (const QList<QVariant>& call : spy)
            names << call.at(0).toString();
        QVERIFY(names.contains(QStringLiteral("UnluacFlags")));
        QVERIFY(names.contains(QStringLiteral("UnluacMode")));
    }

    // ---------------------------------------------------------------- persisted format

    void jsonUsesTheSameKeysAsTheCSharpWriter()
    {
        UserSettings settings;
        const QJsonObject json = settings.toJson();

        // A representative slice of the 68 top-level keys a real AppSettings.json carries.
        for (const char* key : {"ShowChangelog", "OutputDirectory", "GameDirectory", "LastOpenedSettingTab",
                                "IsLoggerExpanded", "AvalonImageSize", "AudioPlayerVolume", "LoadingMode",
                                "LastUpdateCheck", "NextUpdateCheck", "CompressedAudioMode", "AesReload",
                                "DiscordRpc", "AssetLanguage", "CosmeticStyle", "UnluacFlags",
                                "JsonHighlightTheme", "PerDirectory", "ManualGames", "LastAuthResponse",
                                "DirLeftTab", "AssetAddTab", "NextAudio", "MeshExportFormat",
                                "NaniteMeshExportFormat", "CompressionFormat", "LodExportFormat", "CameraMode",
                                "PreviewMaxTextureSize", "SaveHdrTexturesAsHdr",
                                "FeaturePreviewNewAssetExplorer", "PreviewTexturesAssetExplorer"})
        {
            QVERIFY2(json.contains(QLatin1String(key)), key);
        }

        // Enums persist as their underlying integers (Newtonsoft's default, no StringEnumConverter).
        QVERIFY(json[QStringLiteral("LoadingMode")].isDouble());
        QCOMPARE(json[QStringLiteral("LoadingMode")].toInt(), static_cast<int>(ELoadingMode::All));
        QCOMPARE(json[QStringLiteral("AesReload")].toInt(), static_cast<int>(EAesReload::OncePerDay));

        // GridLength persists as a string, via WPF's type converter.
        QVERIFY(json[QStringLiteral("AvalonImageSize")].isString());
        QCOMPARE(json[QStringLiteral("AvalonImageSize")].toString(), QStringLiteral("200"));

        // Hotkeys persist as {"Key": int, "Modifiers": int}.
        const QJsonObject addTab = json[QStringLiteral("AssetAddTab")].toObject();
        QCOMPARE(addTab[QStringLiteral("Key")].toInt(), 63);
        QCOMPARE(addTab[QStringLiteral("Modifiers")].toInt(), 2);
    }

    // The one place the settings tree switches casing: AesResponse/AuthResponse double as wire formats.
    void nestedDtosKeepTheirOwnCasing()
    {
        DirectorySettings directory;
        FModel::ViewModels::ApiEndpoints::Models::AesResponse keys;
        keys.MainKey = QString(66, QLatin1Char('0'));
        directory.setAesKeys(keys);

        const QJsonObject aes = directory.toJson()[QStringLiteral("AesKeys")].toObject();
        QVERIFY(aes.contains(QStringLiteral("mainKey")));    // camelCase, not "MainKey"
        QVERIFY(aes.contains(QStringLiteral("dynamicKeys")));
        QVERIFY(aes[QStringLiteral("dynamicKeys")].isNull()); // C# writes null when there are none

        UserSettings settings;
        FModel::ViewModels::ApiEndpoints::Models::AuthResponse auth;
        auth.AccessToken = QStringLiteral("token");
        auth.ExpiresAt = QDateTime::currentDateTime();
        settings.setLastAuthResponse(auth);

        const QJsonObject stored = settings.toJson()[QStringLiteral("LastAuthResponse")].toObject();
        QVERIFY(stored.contains(QStringLiteral("access_token"))); // snake_case
        QVERIFY(stored.contains(QStringLiteral("expires_at")));
    }

    // A document shaped like a real settings file must survive load -> save -> load unchanged.
    void realWorldDocumentRoundTrips()
    {
        const QJsonObject fixture = makeFixture();

        UserSettings first;
        first.readJson(fixture);

        QCOMPARE(first.gameDirectory(), QStringLiteral("C:/Games/Fortnite/FortniteGame/Content/Paks"));
        QCOMPARE(first.avalonImageSize(), GridLength(730.0));
        QCOMPARE(first.loadingMode(), ELoadingMode::Multiple);
        QCOMPARE(first.aesReload(), EAesReload::OncePerDay);
        QCOMPARE(first.assetAddTab()->key(), Key::T);
        QCOMPARE(first.assetAddTab()->modifiers(), ModifierKeys::Control);
        QCOMPARE(first.perDirectory().size(), 1);

        DirectorySettings* directory = first.perDirectory().value(QStringLiteral("C:/Games/Fortnite/FortniteGame/Content/Paks"));
        QVERIFY(directory != nullptr);
        QCOMPARE(directory->gameName(), QStringLiteral("Fortnite"));
        QCOMPARE(static_cast<quint32>(directory->ueVersion()), 84279296u);
        QCOMPARE(directory->endpoints().size(), 2);
        QCOMPARE(directory->directories().size(), 1);
        QCOMPARE(directory->directories().first()->header(), QStringLiteral("Cosmetics"));
        QCOMPARE(directory->aesKeys().MainKey.length(), 66);
        QVERIFY(directory->aesKeys().isValid());
        QCOMPARE(directory->lastAesReload().date(), QDate(2026, 3, 16));

        // 2^53 + 1: survives only because QJsonValue keeps 64-bit integers exact.
        QCOMPARE(directory->criwareDecryptionKey(), Q_UINT64_C(9007199254740993));

        UserSettings second;
        second.readJson(first.toJson());
        QCOMPARE(second.toJson(), first.toJson());
    }

    void saveAndLoadUseTheFileSystem()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("nested/AppSettings.json"));

        UserSettings source;
        source.readJson(makeFixture());
        QVERIFY(source.saveTo(path)); // also creates the missing "nested" directory
        QVERIFY(QFile::exists(path));

        // Written as indented JSON, matching C#'s Formatting.Indented.
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray contents = file.readAll();
        QVERIFY(contents.contains("\n    \""));

        QScopedPointer<UserSettings> loaded(UserSettings::loadFrom(path));
        QCOMPARE(loaded->toJson(), source.toJson());

        // A missing file yields a default-constructed UserSettings rather than a failure.
        QScopedPointer<UserSettings> missing(UserSettings::loadFrom(dir.filePath(QStringLiteral("absent.json"))));
        QCOMPARE(missing->loadingMode(), ELoadingMode::All);
    }

    // Unknown keys are ignored and absent keys keep their defaults — MissingMemberHandling.Ignore.
    void partialDocumentsKeepDefaults()
    {
        QJsonObject json;
        json[QStringLiteral("ImageMergerMargin")] = 42;
        json[QStringLiteral("SomeKeyFromAFutureVersion")] = QStringLiteral("ignored");

        UserSettings settings;
        settings.readJson(json);
        QCOMPARE(settings.imageMergerMargin(), 42);
        QCOMPARE(settings.previewMaxTextureSize(), 1024); // untouched default
        QCOMPARE(settings.loadingMode(), ELoadingMode::All);
    }

    // ---------------------------------------------------------------- DirectorySettings::Default

    // Re-detecting a game must not discard what the user already configured for that path.
    void directoryDefaultReusesStoredSettings()
    {
        const QString path = QStringLiteral("C:/Games/Fortnite/FortniteGame/Content/Paks");

        auto* store = new UserSettings();
        UserSettings::SetDefault(store); // takes ownership

        auto* stored = new DirectorySettings();
        stored->setGameDirectory(path);
        stored->setUeVersion(static_cast<DirectorySettings::EGame>(84279296u));
        stored->setCriwareDecryptionKey(1234);
        stored->setLastAesReload(QDateTime(QDate(2026, 3, 16), QTime(0, 0)));
        store->setPerDirectory({{path, stored}});

        QScopedPointer<DirectorySettings> rebuilt(
            DirectorySettings::Default(QStringLiteral("Fortnite"), path, false,
                                       CUE4Parse::UE4::Versions::GAME_UE4_LATEST));

        // The stored engine version wins over the passed-in one, and the other tweaks carry over.
        QCOMPARE(static_cast<quint32>(rebuilt->ueVersion()), 84279296u);
        QCOMPARE(rebuilt->criwareDecryptionKey(), Q_UINT64_C(1234));
        QCOMPARE(rebuilt->lastAesReload().date(), QDate(2026, 3, 16));

        // Unlike C#'s MemberwiseClone-style reuse, the subtree is copied, not shared.
        QVERIFY(rebuilt->versioning() != stored->versioning());

        // A directory with nothing stored gets the per-game seed instead.
        QScopedPointer<DirectorySettings> fresh(
            DirectorySettings::Default(QStringLiteral("Fortnite"), QStringLiteral("D:/Elsewhere")));
        QCOMPARE(fresh->directories().size(), 5);
        QCOMPARE(fresh->endpoints().size(), 2);
        QCOMPARE(fresh->lastAesReload().date(), QDate::currentDate().addDays(-1));

        UserSettings::SetDefault(nullptr);
    }

    void directoryEqualityIsDirectoryPlusVersion()
    {
        DirectorySettings a;
        a.setGameName(QStringLiteral("Fortnite"));
        a.setGameDirectory(QStringLiteral("C:/Paks"));
        a.setUeVersion(static_cast<DirectorySettings::EGame>(84279296u));

        DirectorySettings b;
        b.setGameName(QStringLiteral("Something Else Entirely"));
        b.setGameDirectory(QStringLiteral("C:/Paks"));
        b.setUeVersion(static_cast<DirectorySettings::EGame>(84279296u));

        QVERIFY(a.equals(b)); // GameName is deliberately not part of equality

        b.setUeVersion(static_cast<DirectorySettings::EGame>(CUE4Parse::UE4::Versions::GAME_UE4_0));
        QVERIFY(!a.equals(b));
    }

    void directoryCloneIsIndependent()
    {
        DirectorySettings source;
        source.setGameName(QStringLiteral("Fortnite"));
        source.setGameDirectory(QStringLiteral("C:/Paks"));
        source.setDirectories(CustomDirectory::Default(QStringLiteral("Fortnite")));

        QScopedPointer<DirectorySettings> copy(source.clone());
        QCOMPARE(copy->gameName(), source.gameName());
        QCOMPARE(copy->directories().size(), source.directories().size());
        QVERIFY(copy->directories().first() != source.directories().first());

        copy->directories().first()->setHeader(QStringLiteral("Renamed"));
        QCOMPARE(source.directories().first()->header(), QStringLiteral("Cosmetics"));
    }

    // ---------------------------------------------------------------- IsEndpointValid

    void isEndpointValidReadsCurrentDir()
    {
        auto* store = new UserSettings();
        UserSettings::SetDefault(store);

        EndpointSettings* endpoint = nullptr;
        QVERIFY(!UserSettings::IsEndpointValid(EEndpointType::Aes, endpoint)); // no CurrentDir yet
        QVERIFY(endpoint == nullptr);

        auto* directory = new DirectorySettings(store);
        directory->setEndpoints(EndpointSettings::Default(QStringLiteral("Fortnite")));
        store->setCurrentDir(directory);

        QVERIFY(UserSettings::IsEndpointValid(EEndpointType::Aes, endpoint));
        QCOMPARE(endpoint, directory->endpoints().at(0));

        // Overwrite forces validity even when the endpoint itself never validated.
        directory->endpoints().at(1)->setIsValid(false);
        QVERIFY(!UserSettings::IsEndpointValid(EEndpointType::Mapping, endpoint));
        directory->endpoints().at(1)->setOverwrite(true);
        QVERIFY(UserSettings::IsEndpointValid(EEndpointType::Mapping, endpoint));

        UserSettings::SetDefault(nullptr);
    }

    // ---------------------------------------------------------------- VersioningSettings

    void versioningRoundTripsGuidsAndMaps()
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        VersioningSettings source;
        source.setCustomVersions({FCustomVersion(FGuid(0x12345678u, 0x9ABCDEF0u, 0x0FEDCBA9u, 0x87654321u), 7)});
        source.setOptions({{QStringLiteral("SomeOption"), true}, {QStringLiteral("Another"), false}});
        source.setMapStructTypes({{QStringLiteral("MapProp"), {QStringLiteral("KeyType"), QStringLiteral("ValType")}}});

        const QJsonObject json = source.toJson();

        // FGuid carries a JsonConverter that writes the UniqueObjectGuid form.
        const QString guid = json[QStringLiteral("CustomVersions")].toArray().at(0).toObject()[QStringLiteral("Key")].toString();
        QCOMPARE(guid.count(QLatin1Char('-')), 3);
        QCOMPARE(guid.length(), 35);

        VersioningSettings target;
        target.readJson(json);
        QCOMPARE(target.customVersions().size(), 1);
        QCOMPARE(target.customVersions().first().Version, 7);
        QVERIFY(target.customVersions().first().Key == source.customVersions().first().Key);
        QCOMPARE(target.options(), source.options());
        QCOMPARE(target.mapStructTypes(), source.mapStructTypes());
    }

private:
    // A settings document with the same shape as one written by the C# app. The AES key is an obvious
    // placeholder of the right length (0x + 64 hex digits) — never a real one.
    static QJsonObject makeFixture()
    {
        QJsonObject hotkey;
        hotkey[QStringLiteral("Key")] = 63;
        hotkey[QStringLiteral("Modifiers")] = 2;

        QJsonObject aes;
        aes[QStringLiteral("mainKey")] = QStringLiteral("0x") + QString(64, QLatin1Char('A'));
        aes[QStringLiteral("dynamicKeys")] = QJsonValue(QJsonValue::Null);

        QJsonObject endpoint;
        endpoint[QStringLiteral("Url")] = QStringLiteral("https://uedb.dev/svc/api/v1/fortnite/aes");
        endpoint[QStringLiteral("Path")] = QStringLiteral("$.['mainKey','dynamicKeys']");
        endpoint[QStringLiteral("Overwrite")] = false;
        endpoint[QStringLiteral("FilePath")] = QString();
        endpoint[QStringLiteral("IsValid")] = true;

        QJsonObject customDirectory;
        customDirectory[QStringLiteral("Header")] = QStringLiteral("Cosmetics");
        customDirectory[QStringLiteral("DirectoryPath")] =
            QStringLiteral("FortniteGame/Plugins/GameFeatures/BRCosmetics/Content/Athena/Items/Cosmetics");

        QJsonObject versioning;
        versioning[QStringLiteral("CustomVersions")] = QJsonArray();
        versioning[QStringLiteral("Options")] = QJsonObject();
        versioning[QStringLiteral("MapStructTypes")] = QJsonObject();

        QJsonObject directory;
        directory[QStringLiteral("GameName")] = QStringLiteral("Fortnite");
        directory[QStringLiteral("GameDirectory")] = QStringLiteral("C:/Games/Fortnite/FortniteGame/Content/Paks");
        directory[QStringLiteral("IsManual")] = false;
        directory[QStringLiteral("UeVersion")] = 84279296;
        directory[QStringLiteral("TexturePlatform")] = 0;
        directory[QStringLiteral("Versioning")] = versioning;
        directory[QStringLiteral("Endpoints")] = QJsonArray{endpoint, endpoint};
        directory[QStringLiteral("Directories")] = QJsonArray{customDirectory};
        directory[QStringLiteral("AesKeys")] = aes;
        directory[QStringLiteral("LastAesReload")] = QStringLiteral("2026-03-16T00:00:00");
        directory[QStringLiteral("CriwareDecryptionKey")] = Q_INT64_C(9007199254740993);
        directory[QStringLiteral("UnluacOpCodeMap")] = QJsonValue(QJsonValue::Null);

        QJsonObject perDirectory;
        perDirectory[QStringLiteral("C:/Games/Fortnite/FortniteGame/Content/Paks")] = directory;

        QJsonObject json;
        json[QStringLiteral("ShowChangelog")] = true;
        json[QStringLiteral("OutputDirectory")] = QStringLiteral("C:/FModel/Output");
        json[QStringLiteral("GameDirectory")] = QStringLiteral("C:/Games/Fortnite/FortniteGame/Content/Paks");
        json[QStringLiteral("LastOpenedSettingTab")] = 3;
        json[QStringLiteral("IsLoggerExpanded")] = true;
        json[QStringLiteral("AvalonImageSize")] = QStringLiteral("730");
        json[QStringLiteral("AudioPlayerVolume")] = 0.0;
        json[QStringLiteral("LoadingMode")] = 0;
        json[QStringLiteral("LastUpdateCheck")] = QStringLiteral("2026-07-20T23:42:18.117");
        json[QStringLiteral("KeepDirectoryStructure")] = true;
        json[QStringLiteral("AesReload")] = 2;
        json[QStringLiteral("UnluacFlags")] = 3;
        json[QStringLiteral("PerDirectory")] = perDirectory;
        json[QStringLiteral("ManualGames")] = QJsonObject();
        json[QStringLiteral("AssetAddTab")] = hotkey;
        json[QStringLiteral("MeshExportFormat")] = 3;
        json[QStringLiteral("CameraMode")] = 1;
        return json;
    }
};

QTEST_MAIN(TestSettings)
#include "test_settings.moc"
