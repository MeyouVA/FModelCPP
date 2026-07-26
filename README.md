# FModelCPP

A slow, faithful C++ port of [FModel](../FModel) and its parsing core **CUE4Parse** (originally C#).

The goal is to stay as structurally close to the C# source as practical: same folder
layout, same namespaces (`CUE4Parse::UE4::Readers`, ...), same type and method names.

The FModel app (originally WPF) is ported on **Qt 6 Widgets** and lives under `FModel/`. It keeps the same
layout and structure as the C# UI. See [The FModel app](#the-fmodel-app) below.

**[Docs/README.md](Docs/README.md) is the port status & roadmap** — what is done per layer, what remains in
phases, and the recommended next slices. This file is the detail: the per-file porting table and the
catalogue of deliberate differences from the C# source.

## The FModel app

The `FModel/` directory is the C++ port of the WPF application, built on **Qt 6 Widgets** (chosen as the closest
structural analog to WPF: data binding via signals/slots, tree/list/table views, dialogs). It builds only when
Qt6 is found; the CUE4Parse library and tests build fine without it.

- **Toolchain:** Qt 6.8.3 (MSVC 2022 64-bit) installed via `aqtinstall` to `C:\Qt\6.8.3\msvc2022_64`. Configure
  with `-DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64`. The root `CMakeLists.txt` guards the app behind
  `find_package(Qt6 QUIET)` so a Qt-less checkout still builds the library. `windeployqt` runs post-build to copy
  the Qt runtime next to the exe.
- **Ported so far:** `MainWindow` — the window **shell/layout** (`FModel/MainWindow.{h,cpp}`, `main.cpp`), mirroring
  `MainWindow.xaml` closely: the Directory/Packages/Views/Settings/Help menu bar (+ the "Preview New Explorer
  System" toggle); the left **Archives** (Loading Mode combo + Load + GAME ARCHIVES list + INFORMATION),
  **Folders** (summary line + tree + INFORMATION), and **N Packages** (search + breadcrumb + list + INFORMATION)
  tab control — INFORMATION blocks use FModel's value-left / caption-right layout; the right side with the
  explorer / editor-tab-strip ("New Tab" + a `+` add-tab button) stack, the bottom-right preview-toggle buttons, and
  the Log pane (showing FModel's intro lines). Named widgets keep their WPF `x:Name` (as members + `objectName`).
  Menu actions now run through the real `MenuCommand`; the deeper wiring (CUE4Parse provider / thread worker) is
  **not** ported yet, so the arms that need it log which port they are waiting on, and the other controls are
  still inert placeholders.
- **Theme** (`FModel/Theme.{h,cpp}`): reproduces FModel's **AdonisUI Dark** color scheme (App.xaml) as a Qt Fusion
  `QPalette` + global stylesheet — exact AdonisUI layer colors (window `#2A2B34`, panels `#32323F`, insets `#262630`,
  foreground `#f0f0f0`) with FModel's App.xaml accent overrides (accent `#206BD4`, alert `#D49220`, error `#C22B2B`).
  The accent-blue status bar matches the WPF "Ready" state.
- **Resources** (`FModel/Resources/`): copied verbatim from the C# app (107 files — icons, shaders, fonts,
  highlighting definitions) and embedded via `resources.qrc` (AUTORCC), accessible as `:/Resources/…`. The window
  icon and the log toolbar buttons use the real FModel PNGs.
- **Framework** (`FModel/Framework/`): the MVVM base layer ported from `FModel/Framework`. `ViewModel`
  (`INotifyPropertyChanged` + `INotifyDataErrorInfo`/`IDataErrorInfo`) → a `QObject` with a `propertyChanged`
  signal, a templated `setProperty`, and the validation-error dictionary/indexer kept faithfully. `Command`
  (`ICommand`) → an abstract `QObject` with `execute`/`canExecute`/`canExecuteChanged` (`object parameter` →
  `QVariant`). `ViewModelCommand<T>` → a class template whose C# `WeakReference` to the context view-model is
  modelled by Qt's `QPointer<T>` (nulls on destruction). The first concrete view-model on this
  base is `FStatus` (`FModel/Framework/FStatus.{h,cpp}`, ported from `Framework/FStatus.cs`) — it tracks an
  `EStatusKind` + label and derives `IsReady`, and it is wired into `MainWindow`'s status bar (the label binds
  to `Status.Label` through the `propertyChanged` signal, matching the WPF binding). `EStatusKind` and the
  other plain enums live in `FModel/Enums.h` (ported from `Enums.cs`, now including `EAssetCategory`).
  Behaviour is covered by `FModel/tests/test_framework.cpp` (QtTest, registered with ctest).
- **ViewModels** (`FModel/ViewModels/`): `ApplicationViewModel` — the root view-model the window binds to —
  plus `LoadingModesViewModel`, and the `Constants` / `AssetCategoryExtensions` pieces they are built from.
  `MainWindow` now owns the view-model and reads its `FStatus`, its `Categories` (which fill the categories
  combo), and routes the "Preview New Explorer System" toggle and the left tab strip through it. Notes:
  - **The two guarded properties are where a port can silently diverge.** `IsAssetsExplorerVisible` refuses to
    turn *on* while `UserSettings.FeaturePreviewNewAssetExplorer` is off (turning off is never gated), and
    `SelectedLeftTabIndex` **ignores** anything outside `[0, 2]` rather than clamping. A clamping port would
    look identical in the UI and differ on every rejected assignment, so both are covered by tests.
  - **Deferred, each blocked on its own port:** `CUE4ParseViewModel` (1590 C# lines) and with it the
    VFS-registered/mounted/unmounted handlers and `UpdateProvider`; `CustomDirectories`,
    `AesManager`, `AudioPlayer`; `AvoidEmptyGameDirectory`/`AddGameDirectory`/`Restart`
    (they drive the `DirectorySelector` window and a process restart); and the static `Init*` DLL helpers.
    `gameDisplayName()` therefore returns `"Unknown"`, which is also C#''s null branch.
  - **The constructor deliberately does not exit.** C#''s hard-exits the process when no game directory is
    chosen; with the directory selector unported there is nothing to ask, and an unconfigured app must still
    be constructible for the shell and the tests. `titleExtra()` consequently tolerates a null `CurrentDir`,
    and `MainWindow` only appends it to the title once a directory is configured.
  - **`EAssetCategory` packs a base category in its high 16 bits.** `AssetCategoryExtensions` is the only
    place that layout is decoded. C# writes the enum in terms of `AssetCategoryExtensions.CategoryBase` and
    the extensions in terms of the enum; C++ headers cannot have that cycle, so `Enums.h` spells the base
    literally and `AssetCategoryExtensions.h` `static_assert`s the two agree.
  - **`Constants`**: the compile-time values carry over verbatim; the four build-identity ones come from
    `QCoreApplication::applicationFilePath()` plus optional `FMODEL_APP_VERSION`/`FMODEL_APP_COMMIT_ID`
    definitions, and report `"unknown"` rather than inventing a commit id when nothing is injected.
  - **`EGameName`** was added to CUE4Parse (`UE4/Versions/EGame.{h,cpp}`) for the window title: C#''s
    `EGame.ToString()`, generated from the 234 members'' declaration order with the three pure aliases
    collapsed to the first-declared name, as .NET does.
  Covered by `FModel/tests/test_application_viewmodel.cpp`.
- **Commands** (`FModel/ViewModels/Commands/`): `MenuCommand`, `CopyCommand` and `RightClickMenuCommand` on the
  `ViewModelCommand<T>` base, exposed as the three lazy `ApplicationViewModel` properties WPF binds to
  (`??=` → built on first read, owned by the view-model). `MainWindow`'s `onMenuCommand` stand-in is gone — the
  menu `QAction`s carry the same parameter strings WPF puts in `CommandParameter` and run the real command.
  Notes:
  - **Every arm of all three switches is ported, but most of `MenuCommand`'s targets are not.** Those arms
    raise a `deferred(parameter, waitingOn)` signal naming the port they wait on, which `MainWindow` logs;
    the four that fully work are the three external links and *Open Output Directory* (`Process.Start` with
    `UseShellExecute` → `QDesktopServices::openUrl`, behind a static handler seam so a test can observe it).
    The `Settings` arm now opens the real dialog: C#'s `Helper.OpenWindow<AdonisWindow>` is unported, so
    `MenuCommand` gained a second seam (`setOpenWindowHandler`) that `MainWindow` installs, and the arm falls
    back to `deferred` when no host is installed (which is what the command-layer tests see).
    An unrecognised string still does nothing at all, because C#'s switch has no `default`.
  - **`RightClickMenuCommand` is dispatch plus arithmetic.** The 11-trigger → (action, show type, bulk type)
    table, the bulk type → (directory, log word) table, the group-by-directory pass, the export-path builder
    and the folder/asset selection split are all ported and tested; the extraction walk they feed waits on
    `CUE4ParseViewModel`. The
    "more than one asset in this directory" flag is what folds `EBulkType::Auto` into the bulk mask, and
    `Meshes`/`Animations` deliberately share the model directory while logging different words.
  - **The two arms that needed `TreeItem` are ported now that it exists.** `MenuCommand`'s non-string arm
    re-selects a folder (false then true, which is what makes the explorer re-read it), and
    `setFoldersIsExpanded` — the breadth-first collapse / reverse-order expand walk — is public so it can be
    driven without the unported view-model that owns the tree. `ToolBox_Collapse_All` still defers, because
    the *route* to the tree is `CUE4Parse.AssetsFolder`.
  - **`EBulkType` needed operators.** C# gets `|`/`&`/`HasFlag` free on any enum; `Enums.h` now spells them
    out for the one flag enum that uses them.
  - **`object[]` parameters become a `QVariantList`** of `{trigger, selection}`, the selection holding
    `GameFile*`. C# also accepts `GameFileViewModel` (unwrapped to `.Asset`) and `TreeItem` folders; both
    view-models are unported, so those arms are absent and the folder half of the export walk stays empty.
  - **Nothing in the tests writes to the real clipboard** — `CopyCommand`'s line building is split into a
    `buildText` static for that reason, and `execute` is only driven with the payloads that must be ignored.
  Covered by `FModel/tests/test_commands.cpp`.- **Settings** (`FModel/Settings/`): the whole persisted settings tree — `UserSettings` (the root, ~70 stored
  properties), `DirectorySettings` (per game directory), `EndpointSettings`, `VersioningSettings` and
  `CustomDirectory`, plus the DTOs they embed (`AesResponse`, `AuthResponse`, `DetectedGame`). Covered by
  `FModel/tests/test_settings.cpp`. Notes on this layer:
  - **The JSON shape is a compatibility contract, not a design choice.** `%APPDATA%/FModel/AppSettings.json` is
    written by the C# app people already have installed, so the port has to read and rewrite it losslessly.
    C# gets its shape from Newtonsoft's reflective serializer; C++ has no reflection, so every settings type
    carries a hand-written `toJson`/`readJson` pair, with the conventions Newtonsoft applies for free pinned in
    `Extensions/JsonExtensions.h`: PascalCase property names, enums as their **underlying integers**, dates as
    ISO-8601 with offset, and absent keys leaving defaults untouched (`MissingMemberHandling.Ignore`).
  - **Two nested DTOs deliberately break the casing rule.** `AesResponse` uses camelCase (`mainKey`,
    `dynamicKeys`) and `AuthResponse` snake_case (`access_token`) because in C# they carry explicit
    `[JsonProperty]` names — they double as the API wire format. The port keeps both.
  - **`Framework/InputKeys.h` and `Framework/GridLength.h` are WPF stand-ins that exist for the file format.**
    A `Hotkey` persists as `{"Key": <int>, "Modifiers": <int>}` using `System.Windows.Input.Key`'s numeric
    values, and `GridLength` persists as a string via WPF's type converter (`"730"`, `"Auto"`, `"2*"`).
    `Qt::Key` is *not* reused: its values differ, so persisting it would silently corrupt existing hotkeys.
    `test_settings` pins the values that real settings files anchor (`A == 44`, `T == 63`, `Control == 2`).
  - **`Key`'s name table comes from moc, not from a hand-written switch.** `keyName()` has to reproduce C#'s
    `Key.ToString()` for ~170 enumerators. The enum is registered with `Q_NAMESPACE`/`Q_ENUM_NS` and the names
    read back via `QMetaEnum::valueToKey`, so they are derived from the enum declaration itself and cannot
    drift. (An X-macro was tried first and abandoned: MSVC leaves the callback macro unexpanded at one of two
    otherwise identical invocation sites, even though `/P` preprocesses the same file correctly.)
  - **`[Description]` is modelled as an overload set.** C#'s attribute becomes one `description(E)` function
    per enum (`Description(E)` returning `const char*` on the CUE4Parse side, which stays Qt-free). This is
    what the earlier `FModel/Enums.h` comment deferred.
  - **`DirectorySettings::clone()` deep-copies where C# shallow-copies.** C#'s `ICloneable.Clone()` is
    `MemberwiseClone()`, so a clone *shares* its `Versioning`/`Endpoints`/`Directories` instances with the
    original. In C++ those are owned by their parent, and sharing them would mean a double-delete or a dangler.
    `DirectorySettings::Default()` copies out of the stored entry for the same reason. Visible difference:
    editing a clone no longer writes through to the original — which is what `GameSelectorViewModel`, the only
    caller, actually wants.
  - **Deferred:** `UserSettings.ExportOptions` (projects onto CUE4Parse-Conversion's unported `ExporterOptions`;
    every field it reads is present) and `EndpointSettings.TryValidate` (needs `DynamicApiEndpoint` + HTTP).
  - **Verified against a real file.** `test_settings` runs on a synthetic fixture by default so the suite stays
    hermetic and reproducible — a real `AppSettings.json` holds live AES keys. Point `FMODEL_SETTINGS_FIXTURE`
    at one to additionally assert that load → save → load is a fixed point and that no key is dropped; this was
    run against a real 38-directory file, which passes.
- **The Settings dialog** — `ViewModels/SettingsViewModel.{h,cpp}`, `Views/SettingsView.{h,cpp}` and the two
  editors it hosts, `Views/Resources/Controls/{DictionaryEditor,EndpointEditor}.{h,cpp}`. This is the first
  `MenuCommand` arm to stop being deferred, and the first *editing* surface over the settings tree: the
  view-model snapshots ~25 settings on `Initialize`, the view builds the four tabs (General / Models & Anims /
  Textures & Audio / Advanced) out of those, and `Save` diffs the snapshots to decide what the app must reload.
  Covered by `FModel/tests/test_settings_view.cpp` (19 slots). Notes:
  - **`Save` returns "what should I do" *and* a restart flag, and the restart test is reference identity.**
    C# compares the three versioning collections with `!=` on `IList`/`IDictionary`, so OK'ing a
    `DictionaryEditor` forces a restart **even when the JSON is unchanged** — the editor hands back a new
    object. C++ values would compare equal, so the port carries three `_customVersionsReplaced` /
    `_optionsReplaced` / `_mapStructTypesReplaced` flags set only on the public setters (the editor's path) and
    bypassed by `initialize()`. Same observable behaviour, spelled out instead of implied.
  - **Two upstream slips are reproduced, not fixed.** `Initialize()` assigns
    `SelectedCompressionFormat = _selectedCompressionFormat` — the still-default backing field where every
    neighbouring line uses the snapshot — so the compression combo opens on `None` and OK persists that. And
    the mappings watcher is sticky (`if (!_mappingsUpdate) _mappingsUpdate = …`), so undoing an edit before OK
    still reloads mappings. Both are pinned by tests so a later "cleanup" can't quietly change behaviour.
  - **`EnumerateUeGames` needed `Enum.GetValues<EGame>()`.** `EGame.{h,cpp}` gained `EGameValues()`, a table
    kept in lockstep with the existing `EGameName` switch (generated from it, so the two cannot drift). The
    C# pipeline is `GroupBy((int) value).Select(First)` then `OrderBy(v => ((int) v & 0xFF) == 0)` — a
    **stable** sort putting game-specific members before base engine versions; the port does the same in two
    passes rather than sorting.
  - **`EnumExtensions` is ported only where it has a call site.** `GetDescription`'s *fallback* is the whole
    point: `EGame` carries no `[Description]`, so the description is built by masking `~0xFFFF` and stepping
    back `current - target` **array positions** to name the base version — "GAME_ArkSurvivalEvolved
    (GAME_UE4_5)", while a base version renders its own decimal, "GAME_UE5_3 (84934656)".
  - **`DictionaryEditor` round-trips through a scratch `VersioningSettings`** rather than duplicating the
    `FGuid` JSON converter, which also guarantees the text it shows matches `AppSettings.json`'s shape exactly.
    C# leaves all three collections **null** by default and each ctor writes `collection ?? _defaultX`, which
    is why an unconfigured game shows a sample document; C++ values start empty, so emptiness stands in for
    null. Broken JSON does not close the dialog — it turns the label red, verbatim.
  - **`EndpointEditor` mutates the live `EndpointSettings`** and gates OK on `_isTested && endpoint->isValid()`.
    `OnTest` deliberately does **not** set `_isTested` upstream (only a successful *send* does), so a "tested"
    endpoint that was never sent still can't be OK'd; `onSend`/`onTest` raise `deferred` because the HTTP
    layer is unported.
  - **Deferred inside `OnClick`:** the restart warning, the four `CUE4ParseViewModel` reload steps
    (`ClearProvider`/`LoadLocalizedResources`/mappings) and the provider's `ReadScriptData`/`ReadShaderMaps`
    flags. The dialog still writes `UserSettings::Save()` itself, so the file is correct either way.
  - **`TryParseKey` keeps its dead statement.** Upstream computes
    `int numberBase = text.All(Uri.IsHexDigit) ? 16 : 10;` and never uses it — the parse keys off `isHex`
    alone, and `ulong.TryParse` zeroes its `out` on failure, which the port reproduces.
  - **The JSON theme preview keeps its literal escapes.** C# raw string literals (`"""…"""`) do not process
    escapes, so the preview text really contains the two characters `\n`. A C++ `R"(...)"` behaves the same;
    a test pins it so nobody "fixes" it into a line break.
  - **Deliberate view-layer differences (Settings window):** no `ApplicationService` locator (the view is handed its settings),
    explicit bindings instead of XAML ones, the Creator entry stays hidden (its view-model is unported), a
    `browseHandler` seam so the directory pickers are testable, a non-recolouring theme preview, and
    `MainWindow` runs the dialog **modal** where C# shows it modeless behind `Helper`'s single-instance guard.
- **The explorer's folder tree** — `ViewModels/AssetsFolderViewModel.{h,cpp}` (which holds both `TreeItem` and
  `AssetsFolderViewModel`, as the C# file does), `ViewModels/AssetsListViewModel.{h,cpp}` and the row
  view-model `ViewModels/GameFileViewModel.{h,cpp}`, sitting on `Framework/RangeObservableCollection.h` and a
  new `Framework/CollectionView.h`. One method — `bulkPopulate` — turns the provider's flat file list into
  the nested structure the left pane binds to. Covered by `FModel/tests/test_assets_folder.cpp` (22 slots).
  Notes:
  - **`Framework/CollectionView.h` has no C# counterpart.** It stands in for WPF's
    `ICollectionView`/`ListCollectionView` the way `Utils/Json.h` stands in for Newtonsoft: a sort
    comparison, an optional filter, `refresh()`, and invalidation driven by the source collection's change
    signal. C# names a property in a `SortDescription` and WPF reflects on it; C++ has no reflection, so the
    comparison is passed directly. Grouping, currency and the editing transactions are not modelled because
    nothing in the ported tree uses them.
  - **The suppression flag is the whole performance story, and it is observable.** `bulkPopulate` builds the
    tree with every collection muted and only un-mutes it depth-first at the end, so each view materialises
    once instead of once per file. A view is *not* invalidated while its source is muted — which is why the
    un-mute has to come before the hand-raised `InvokeOnCollectionChanged`, and why a port that skipped the
    recursion would leave every collection below the first level permanently stale. Both are asserted.
  - **`AddRange` clears the suppression flag it set, even if the caller had set it first.** That is C#'s
    ordering, and `bulkPopulate` depends on it: the roots collection is re-armed by the `AddRange` itself.
  - **A path with no `/` at all goes into a synthetic `Content` bucket**, shared by every such file, with a
    null `Parent`. Everything else contributes `folders[0 .. n-2]` as nodes — the file name is never a node,
    which is what makes `PathAtThisPoint` correct for the export paths built from it.
  - **The root selection happens after publishing, on purpose.** Upstream's comment says why: selecting a
    detached `TreeItem` lets WPF auto-select the first root instead. The project-name lookup that picks
    *which* root is a service-locator read in C#, so it is a static seam here; with no name, C#'s
    `?? treeItems[0]` fallback picks the first root built.
  - **`GameFileViewModel` is ported by extension only.** The extension switch — 27 data types, the byte-code
    and audio and soundbank and font sets, and the six game-gated arms — is complete and tested. The other
    resolver loads the asset as a package and runs a ~60-arm type switch over its main export; that needs
    `CUE4ParseViewModel`, the concrete `UExport` tree and a texture decoder, so the row is marked resolved
    and `deferred` is raised. The two extension shortcuts C# takes *before* touching the provider (`umap` →
    World, `_BuiltData` → MapBuildDataRegistry) are ported.
  - **Another upstream slip preserved:** the image arm writes `Resolved |= ~EResolveCompute.Preview`. C#'s
    `~` runs on the underlying int and is not masked to the declared bits, so that ORs in every bit *except*
    Preview rather than clearing it, leaving `Resolved` at `~0`. The visible consequence is that
    `Resolved == EResolveCompute.All` is then false, so `OnIsVisible` never takes its early return for an
    image row. A `&= ~` was presumably meant; a test pins the current behaviour.
  - **The `AssetCategory` setter marks the row's category resolved unconditionally** — outside the "did it
    change" guard, per upstream's own comment. The extension switch's `default:` arm relies on it: it assigns
    `All`, the value the row already has, purely for that side effect.
  - **Deferred:** `PreviewImage`/`NumTextures` (written only by the two decode arms), `ExtractAsync` (needs
    `ThreadWorkerViewModel`), `SearchVm.ChangeCollection`, and the WPF dispatcher marshalling around
    `CombinedEntries` and `BulkPopulate`, which has no counterpart here.

- **The game-loading pipeline** — `ViewModels/CUE4ParseViewModel.{h,cpp}` (the provider host),
  `ViewModels/ThreadWorkerViewModel.{h,cpp}` + `Framework/AsyncQueue.h` (the job funnel every step runs
  through), `ViewModels/GameDirectoryViewModel.{h,cpp}` (the Archives tab), `ViewModels/AesManagerViewModel.{h,cpp}`
  + `Framework/FullyObservableCollection.h`, `Helper.{h,cpp}`, `ViewModels/Commands/LoadCommand.{h,cpp}`,
  the rest of `ViewModels/GameSelectorViewModel.{h,cpp}`, and the two windows the flow needs —
  `Views/DirectorySelector.{h,cpp}` and `Views/AesManager.{h,cpp}`. `ApplicationViewModel` gained
  `CUE4Parse`/`AesManager`/`ThreadWorker`, the three Vfs event handlers and `updateProvider`; `MainWindow`
  runs the startup sequence and binds the Archives list, the Load button and the folder tree. Covered by
  `FModel/tests/test_loading.cpp` (20 slots) **and by three retail installs** (see `Docs/README.md`). Notes:
  - **`CUE4ParseViewModel` is ported by half, on purpose.** The C# file is ~1,800 lines; the *loading* half
    is here (provider selection, `Initialize`, `LoadVfs`, `ClearProvider`, the local-file mappings branch,
    `LoadVirtualPaths`, `LoadLocalizedResources`, `VerifyConsoleVariables`). Everything from
    `ExtractSelected` down — extraction, export, the JSON/metadata/references tabs, Lua decompilation, audio,
    the Snooper viewport — is not, and each is called out at its site.
  - **Everything runs on the calling thread.** C# hands each job to the thread pool
    (`await Task.Run(() => job(token))`) and the UI stays live; the port has no threading layer, so
    `ThreadWorkerViewModel::begin` runs the job synchronously. The queue, the status transitions, the
    cancellation token and the "already busy" signal all behave as upstream — only the thread is missing.
    One consequence is directly observable and pinned by a test: because the status is `Loading` for the
    whole drain, a job that calls `begin()` again is *refused*, not queued. That is C#'s guard too, and it is
    what stops a second Load while one is running.
  - **Two live-service entries are rejected rather than misread.** `fortnite-live.manifest` and
    `valorant-live.manifest` are sentinel "directories" that C# turns into a `StreamedFileProvider` fed by
    Epic's manifest parser. None of that is ported, so the constructor leaves the provider null and says so —
    pointing a `DefaultFileProvider` at a non-existent path would have looked like a broken install.
  - **The launcher auto-detection is not ported.** C#'s `EnumerateDetectedGames` probes every drive for
    Epic's `LauncherInstalled.dat`, Riot's installs JSON, Steam's `libraryfolders.vdf` + `.acf` manifests,
    and two registry keys, to pre-populate ~20 games. That is platform-specific format parsing with no
    bearing on the port's spine; a user browses to a directory instead. **`TryDetectUeVersion` IS ported**,
    including its walk down to a `Paks` folder and up to `Binaries\Win64`, reading the UE version out of the
    exe's Win32 version resource (C# reads the same data through `FileVersionInfo`).
  - **`Helper::fixKey` keeps its off-by-two.** The guard is `keySpan.Length > sizeof(char) * (2 + 32)`, and
    `sizeof(char)` is **2** in C#, so the limit is 68 characters rather than the 66 a prefixed 256-bit key
    occupies. A 67- or 68-character key passes and comes back longer than any key can be, instead of being
    rejected as a "bullshit key". Pinned by a test.
  - **`AesManagerViewModel`'s rows ARE the Archives tab's rows.** `EnumerateAesKeys` yields the very same
    `FileItem` objects, after stamping each with the key from settings — so editing a key in the AES manager
    mutates the archive list's item too. That is upstream's design, and it is what
    `FullyObservableCollection` (which forwards an *item's* property change tagged with its index) exists to
    observe.
  - **`LoadCommand`'s `Multiple` mode is sticky.** An empty selection falls through to `All` via C#'s
    `goto case`, which also writes `ELoadingMode.All` back into settings — so one empty-selection load
    silently changes the user's loading mode for good. Kept. `AllButNew`/`AllButModified` read a `.fbkp`
    backup through an LZ4 frame decoder and are deferred; `All`, `Multiple` and `AllButPatched` are ported.
  - **The mappings step is guarded, and C# does not guard it.** A real `.usmap` in a real install
    (Satisfactory's `CommunityResources` copy) throws part-way through `UsmapParser`; C# runs `InitMappings`
    inside `Task.Run` and lets the failure escape into an `async void`, which the dispatcher swallows. Here
    an escaping exception would take the process down, and a broken mappings file must not stop a game that
    has already mounted — so `MainWindow` catches and logs it. The parser defect itself is a separate,
    recorded next slice.
  - **`UserSettings` gained `addPerDirectory`/`removePerDirectory`.** C# indexes the dictionary directly
    (`PerDirectory[dir] = setting`); the map owns its values here, so a replaced or removed entry has to be
    deleted — and `CurrentDir` cleared first if it pointed at it.
  - **`SettingsViewModel::enumerateUeGames` now delegates.** C# declares that method twice, character for
    character, on `SettingsViewModel` and on `GameSelectorViewModel`. Rather than duplicate it, the
    implementation lives on the latter (lower in the build's layering) and the former calls through.

`CUE4Parse/CUE4Parse-Conversion/` is a new project sitting alongside `CUE4Parse/CUE4Parse/` inside the same
checkout, mirroring the C# repo layout (both targets are defined by `CUE4Parse/CMakeLists.txt`). It holds, so
far, only the export-format enums (`EMeshFormat`, `ELodFormat`, `ESocketFormat`, `ETextureFormat`,
`EAnimFormat`, `EFileCompressionFormat`) that `UserSettings` persists. It is a header-only INTERFACE target
that becomes a static library as soon as the first `.cpp` lands.

The FModel UI (WPF) is being ported phase-by-phase alongside the CUE4Parse core below.

## Status

Port is being done bottom-up, starting with CUE4Parse. Done so far — the **version + reader
foundation** everything else builds on:

| Area | C# source | C++ port |
| --- | --- | --- |
| Alignment helpers | `Utils/AlignUtils.cs` | `Utils/AlignUtils.h` |
| Engine/game enum | `UE4/Versions/EGame.cs` | `UE4/Versions/EGame.{h,cpp}` |
| Object version enums | `UE4/Versions/ObjectVersion.cs` | `UE4/Versions/ObjectVersion.h` |
| Package file version | `UE4/Versions/ObjectVersion.cs` | `UE4/Versions/FPackageFileVersion.h` |
| Version container | `UE4/Versions/VersionContainer.cs` | `UE4/Versions/VersionContainer.{h,cpp}` |
| Custom-version lookup | `UE4/Versions/VersionUtils.cs` | `UE4/Versions/VersionUtils.{h,cpp}` |
| Custom version families (40) | `UE4/Versions/F*{Object,Custom}Version.cs` | `UE4/Versions/F*{Object,Custom}Version.h` |
| Asset registry version | `UE4/Versions/FAssetRegistryVersion.cs` | `UE4/Versions/FAssetRegistryVersion.h` |
| Texture platform | `UE4/Assets/.../ETexturePlatform.cs` | `UE4/Assets/Exports/Texture/ETexturePlatform.h` |
| Wwise enums (36) | `UE4/Wwise/Enums/*.cs` | `UE4/Wwise/Enums/*.h` |
| Wwise flag enums (15) | `UE4/Wwise/Enums/Flags/*.cs` | `UE4/Wwise/Enums/Flags/*.h` |
| FMod enums (16) | `UE4/FMod/Enums/*.cs` | `UE4/FMod/Enums/*.h` |
| Remaining enum-only files (46) | across the tree (`Kismet`, `RigVM`, `RHI`, `i18N`, `Texture`, `Material`, `Animation`, `WorldPartition`, `VirtualFileCache`, …) | same relative paths, `.h` |
| Wwise archive + version info | `UE4/Wwise/WwiseArchive.cs`, `WwiseVersionInfo.cs`, `WwiseFnv.cs` | `UE4/Wwise/WwiseArchive.h`, `WwiseVersionInfo.h`, `WwiseFnv.h` |
| Wwise bank objects (57) | `UE4/Wwise/Objects/*.cs` | `UE4/Wwise/Objects/*.h` |
| Wwise actions (15) | `UE4/Wwise/Objects/Actions/*.cs` | `UE4/Wwise/Objects/Actions/*.h` |
| Wwise plugin params (~50) | `UE4/Wwise/Plugins/**/*.cs`, `WwisePlugin.cs` | `UE4/Wwise/Plugins/**/*.h`, `UE4/Wwise/WwisePlugin.h` |
| Wwise HIRC hierarchy (33) | `UE4/Wwise/Objects/HIRC/**/*.cs` | `UE4/Wwise/Objects/HIRC/**/*.h` |
| Bulk data | `UE4/Assets/Objects/{TBulkData,FByteBulkData,FByteBulkDataHeader,FIntBulkData,FColorBulkData}.cs` | same names, `.h` (+ `FByteBulkDataHeader.cpp`) |
| Data resources | `UE4/Objects/UObject/FObjectDataResource.cs` | `UE4/Objects/UObject/FObjectDataResource.{h,cpp}` |
| Property accessors | `UE4/Assets/Exports/UObject.cs` (PropertyUtil) | `UE4/Assets/Exports/PropertyUtil.{h,cpp}` |
| Wwise reader | `UE4/Wwise/WwiseReader.cs`, `Objects/AkEntry.cs`, `FDeferredByteData.cs` | `UE4/Wwise/WwiseReader.{h,cpp}`, `Objects/AkEntry.h`, `FDeferredByteData.h` |
| Wwise asset types (41) | `UE4/Assets/Exports/Wwise/*.cs` | `UE4/Assets/Exports/Wwise/*.h` |
| FMod asset types (7) | `UE4/Assets/Exports/FMod/*.cs` | `UE4/Assets/Exports/FMod/*.h` |
| FMod bank tree (106) | `UE4/FMod/**/*.cs` | `UE4/FMod/**/*.h` (+ `FModReader.cpp`) |
| Wwise provider | `UE4/Wwise/WwiseProvider.cs` | `UE4/Wwise/WwiseProvider.{h,cpp}` |
| FMod provider | `UE4/FMod/FModProvider.cs` | `UE4/FMod/FModProvider.{h,cpp}` |
| Sound export tree (22) | `UE4/Assets/Exports/Sound/**/*.cs` | `UE4/Assets/Exports/Sound/**/*.h` (+ `USoundWave.cpp`) |
| MetaSound export tree (24) | `UE4/Assets/Exports/MetaSound/*.cs` | `UE4/Assets/Exports/MetaSound/*.h` |
| Parser exceptions | `UE4/Exceptions/ParserException.cs` | `UE4/Exceptions/ParserException.{h,cpp}` |
| FName (full) | `UE4/Objects/UObject/FName.cs` | `UE4/Objects/UObject/FName.h` |
| Base archive | `UE4/Readers/FArchive.cs` | `UE4/Readers/FArchive.{h,cpp}` |
| Byte archive | `UE4/Readers/FByteArchive.cs` | `UE4/Readers/FByteArchive.{h,cpp}` |
| Pointer archive | `UE4/Readers/FPointerArchive.cs` | `UE4/Readers/FPointerArchive.{h,cpp}` |
| Big-endian archive | `UE4/Readers/FArchiveBigEndian.cs` | `UE4/Readers/FArchiveBigEndian.{h,cpp}` |
| Stream / file archives | `UE4/Readers/FStreamArchive.cs` | `UE4/Readers/FStreamArchive.{h,cpp}` |
| Compressed-load proxy | `UE4/Readers/FArchiveLoadCompressedProxy.cs` | `UE4/Readers/FArchiveLoadCompressedProxy.{h,cpp}` |
| Compression method enum | `Compression/CompressionMethod.cs` | `Compression/CompressionMethod.h` |
| Compression flags enum | `UE4/.../Core/Misc/ECompressionFlags.cs` | `UE4/Objects/Core/Misc/ECompressionFlags.h` |
| Decompression core | `Compression/Compression.cs` | `Compression/Compression.{h,cpp}` |
| Unknown-method exception | `UE4/.../UnknownCompressionMethodException.cs` | `UE4/Exceptions/UnknownCompressionMethodException.h` |
| CityHash | `Utils/CityHash.cs` | `Utils/CityHash.{h,cpp}` |
| CRC32 | `Utils/CRC32.cs` | `Utils/CRC32.{h,cpp}` |
| Math helpers (scalar) | `Utils/MathUtils.cs` | `Utils/MathUtils.h` |
| Hex parsing | `Utils/HexUtils.cs` | `Utils/HexUtils.h` |
| String helpers | `Utils/StringUtils.cs` | `Utils/StringUtils.h` |
| Array / bit helpers | `Utils/ArrayUtils.cs` | `Utils/ArrayUtils.h` |
| UStruct marker | `UE4/IUStruct.cs` | `UE4/IUStruct.h` |
| 128-bit uint | `UE4/.../Core/Math/FUInt128.cs` | `UE4/Objects/Core/Math/FUInt128.h` |
| GUID | `UE4/.../Core/Misc/FGuid.cs` | `UE4/Objects/Core/Misc/FGuid.h` |
| Timestamp | `UE4/.../Core/Misc/FDateTime.cs` | `UE4/Objects/Core/Misc/FDateTime.h` |
| Frame number | `UE4/.../Core/Misc/FFrameNumber.cs` | `UE4/Objects/Core/Misc/FFrameNumber.h` |
| Frame rate | `UE4/.../Core/Misc/FFrameRate.cs` | `UE4/Objects/Core/Misc/FFrameRate.h` |
| SHA-1 hash | `UE4/.../Core/Misc/FSHAHash.cs` | `UE4/Objects/Core/Misc/FSHAHash.h` |
| Engine version (base) | `UE4/.../Core/Misc/FEngineVersionBase.cs` | `UE4/Objects/Core/Misc/FEngineVersionBase.h` |
| Engine version | `UE4/.../Core/Misc/FEngineVersion.cs` | `UE4/Objects/Core/Misc/FEngineVersion.h` |
| Package flags | `UE4/.../UObject/EPackageFlags.cs` | `UE4/Objects/UObject/EPackageFlags.h` |
| Compressed chunk | `UE4/Assets/Objects/FCompressedChunk.cs` | `UE4/Assets/Objects/FCompressedChunk.h` |
| Custom version | `UE4/.../Serialization/FCustomVersion.cs` | `UE4/Objects/Core/Serialization/FCustomVersion.h` |
| Custom version container | `UE4/.../Serialization/FCustomVersionContainer.cs` | `UE4/Objects/Core/Serialization/FCustomVersionContainer.{h,cpp}` |
| Licensee file version | `UE4/Versions/ObjectVersion.cs` | `UE4/Versions/ObjectVersion.h` (enum added) |
| Package file summary | `UE4/.../UObject/FPackageFileSummary.cs` | `UE4/Objects/UObject/FPackageFileSummary.{h,cpp}` |
| Object flags | `UE4/Assets/Exports/EObjectFlags.cs` | `UE4/Assets/Exports/EObjectFlags.h` |
| Mapped name (IO Store) | `UE4/IO/Objects/FMappedName.cs` | `UE4/IO/Objects/FMappedName.h` |
| Serialized name entry | `UE4/.../UObject/FNameEntrySerialized.cs` | `UE4/Objects/UObject/FNameEntrySerialized.{h,cpp}` |
| Package interface (min) | `UE4/Assets/IPackage.cs` | `UE4/Assets/IPackage.h` (NameMap + HasFlags subset) |
| Asset archive | `UE4/Assets/Readers/FAssetArchive.cs` | `UE4/Assets/Readers/FAssetArchive.{h,cpp}` |
| Package index | `UE4/.../UObject/ObjectResource.cs` | `UE4/Objects/UObject/ObjectResource.h` |
| Object import/export | `UE4/.../UObject/ObjectResource.cs` | `UE4/Objects/UObject/ObjectResource.{h,cpp}` |
| Resolved object | `UE4/Assets/AbstractUePackage.cs` | `UE4/Assets/ResolvedObject.{h,cpp}` (names/paths, no loading) |
| Package reader | `UE4/Assets/Package.cs` | `UE4/Assets/Package.{h,cpp}` (header + resolution) |
| Property tag data | `UE4/Assets/Objects/FPropertyTagData.cs` | `UE4/Assets/Objects/FPropertyTagData.{h,cpp}` |
| Property tag | `UE4/Assets/Objects/FPropertyTag.cs` | `UE4/Assets/Objects/FPropertyTag.{h,cpp}` (classic layout) |
| Property value base | `UE4/.../Properties/FPropertyTagType.cs` | `UE4/Assets/Objects/Properties/FPropertyTagType.{h,cpp}` |
| Scalar properties | `UE4/.../Properties/*Property.cs` | `UE4/Assets/Objects/Properties/*Property.h` (13 scalar types) |
| Struct value / fallback | `UE4/Assets/Objects/FScriptStruct.cs`, `FStructFallback.cs` | `UE4/Assets/Objects/FScriptStruct.{h,cpp}` (named-struct table for the ported types), `FStructFallback.{h,cpp}` |
| Config ini parser | *(the `Infrablack.UE4Config` NuGet dependency)* | `UE4Config/Parsing/{ConfigIni.{h,cpp},IniToken.h}`, `UE4Config/Evaluation/PropertyEvaluator.{h,cpp}` (read path, vendored) |
| Array value | `UE4/Assets/Objects/UScriptArray.cs` | `UE4/Assets/Objects/UScriptArray.{h,cpp}` |
| Map / Set value | `UE4/Assets/Objects/UScript{Map,Set}.cs` | `UE4/Assets/Objects/UScript{Map,Set}.{h,cpp}` |
| Struct/Array/Enum properties | `UE4/.../Properties/{Struct,Array,Enum}Property.cs` | `UE4/Assets/Objects/Properties/{Struct,Array,Enum}Property.{h,cpp}` |
| Map/Set/Object properties | `UE4/.../Properties/{Map,Set,Object}Property.cs` | `UE4/Assets/Objects/Properties/{Map,Set,Object}Property.{h,cpp}` |
| Soft object path / unique guid | `UE4/.../UObject/FSoftObjectPath.cs`, `FUniqueObjectGuid.cs` | `UE4/Objects/UObject/FSoftObjectPath.{h,cpp}`, `FUniqueObjectGuid.h` |
| Soft / object-ish properties | `UE4/.../Properties/{SoftObject,Weak,Lazy,Class,AssetObject}Property.cs` | `UE4/Assets/Objects/Properties/{SoftObject,WeakObject,LazyObject,Class,AssetObject}Property.{h,cpp}` |
| Script delegate / interface / field path | `UE4/.../UObject/{ScriptDelegates,ScriptInterface,FFieldPath}.cs` | `UE4/Objects/UObject/{ScriptDelegates,ScriptInterface,FFieldPath}.{h,cpp}` |
| Delegate / interface / field-path / string-variant properties | `UE4/.../Properties/{Delegate,MulticastDelegate,Interface,FieldPath,AnsiStr,Utf8Str,VerseString}Property.cs` | `UE4/Assets/Objects/Properties/{Delegate,MulticastDelegate,Interface,FieldPath,AnsiStr,Utf8Str,VerseString}Property.{h,cpp}` |
| FText + FTextHistory (i18N) | `UE4/.../Core/i18N/FText.cs` | `UE4/Objects/Core/i18N/FText.{h,cpp}` |
| Text / Optional properties | `UE4/.../Properties/{Text,Optional}Property.cs` | `UE4/Assets/Objects/Properties/{Text,Optional}Property.{h,cpp}` |
| UObject (core) | `UE4/Assets/Exports/UObject.cs` | `UE4/Assets/Exports/UObject.{h,cpp}` (tagged path) |
| Lazy export loading | `UE4/Assets/AbstractUePackage.cs`, `Package.cs` | `UE4/Assets/Package.{h,cpp}` (`GetExportObject`/`ConstructObject`) |
| File provider (min) | `FileProvider/IFileProvider.cs`, `InternationalizationDictionary.cs` | `FileProvider/IFileProvider.h`, `InternationalizationDictionary.h` (lookup + TryLoadPackage subset) |
| Cross-package import resolution | `UE4/Assets/AbstractUePackage.cs` (`ResolveImport`) | `UE4/Assets/Package.cpp` (`ResolveImport` via provider) |
| Object type registry | `UE4/Assets/ObjectTypeRegistry.cs` | `UE4/Assets/ObjectTypeRegistry.{h,cpp}` (manual, reflection-free) |
| UStringTable + FStringTable | `UE4/.../Exports/Internationalization/{UStringTable,FStringTable}.cs` | `UE4/Assets/Exports/Internationalization/{UStringTable,FStringTable}.{h,cpp}` |
| UDataTable | `UE4/Assets/Exports/Engine/UDataTable.cs` | `UE4/Assets/Exports/Engine/UDataTable.{h,cpp}` |
| UCurveTable | `UE4/Assets/Exports/Engine/UCurveTable.cs` | `UE4/Assets/Exports/Engine/UCurveTable.{h,cpp}` (+ `ECurveTableMode.h`) |
| Curve eval subsystem | `UE4/Objects/Engine/Curves/{RealCurve,SimpleCurve,RichCurve}.cs` | same paths under `FModelCPP/.../Curves/*.{h,cpp}` |
| UCurve* assets | `UE4/Objects/Engine/Curves/{UCurveBase,UCurveFloat,UCurveVector,UCurveLinearColor}.cs` | same paths under `FModelCPP/.../Curves/*.{h,cpp}` |
| Math: UnrealMath + CubicCurve2D | `UE4/.../Math/UnrealMathUtility.cs`, `Utils/MathUtils.cs` | `UE4/Objects/Core/Math/UnrealMathUtility.h`, `Utils/MathUtils.h` |
| Colors: FColor + FLinearColor | `UE4/.../Core/Math/{FColor,FLinearColor}.cs`, `Utils/UnsafePrint.cs` | `UE4/Objects/Core/Math/{FColor.h,FLinearColor.{h,cpp}}`, `Utils/UnsafePrint.h` |
| Vectors: FVector + FVector2D/FVector4 + int/packed vectors | `UE4/.../Core/Math/{FVector,FVector2D,FVector4,FIntVector,FIntVector2,FUIntVector,FIntPoint,FVector3SignedShortScale,FTwoVectors,FCapsuleShape,FFloat16,TIntPoint,TPair}.cs` | same paths under `UE4/Objects/Core/Math/*.h` (+ `FVector.cpp`) |
| Geometry: FQuat/FRotator/FMatrix/FTransform/FPlane | `UE4/.../Core/Math/{FQuat,FRotator,Matrix,FTransform,FPlane,RotationMatrix,RotationTranslationMatrix,QuatRotationTranslationMatrix}.cs` | same paths under `UE4/Objects/Core/Math/*.h` (+ `{Matrix,FQuat,FRotator}.cpp`, `EForceInit.h`) |
| Bounds + ranges: FBox/FSphere/FBoxSphereBounds/TRange | `UE4/.../Core/Math/{FBox,FBox2D,TBox3,FSphere,FBoxSphereBounds,TInterval,TRange,TRangeBound,TIntVector,FHalfVector,FVector3UnsignedShort}.cs` | same paths under `UE4/Objects/Core/Math/*.h` |
| AES-256 decryption | `Encryption/Aes/{Aes,FAesKey}.cs` | `Encryption/Aes/{Aes.{h,cpp},FAesKey.h}` (inverse cipher written from scratch) |
| Frozen/memory-image reader | `UE4/Readers/FMemoryImageArchive.cs` | `UE4/Readers/FMemoryImageArchive.{h,cpp}` |
| GameFile + virtual file system | `FileProvider/Objects/GameFile.cs`, `UE4/VirtualFileSystem/{IVfsReader,IAesVfsReader,VfsEntry,AbstractVfsReader,AbstractAesVfsReader}.cs` | same paths under `FileProvider/Objects/` and `UE4/VirtualFileSystem/` (+ `Utils/StringComparer.h`) |
| Pak container | `UE4/Pak/PakFileReader.cs`, `UE4/Pak/Objects/{FPakInfo,FPakEntry,FPakCompressedBlock}.cs` | same paths under `UE4/Pak/` |
| FField / FProperty system | `UE4/Objects/UObject/{FField,UnrealType}.cs` | `UE4/Objects/UObject/{FField,UnrealType}.{h,cpp}` |
| UStruct family | `UE4/Objects/UObject/{UField,UStruct,UScriptStruct,UEnum,UFunction,UClass}.cs` | same paths under `FModelCPP/.../UObject/*.{h,cpp}` |
| UObjectRedirector | `UE4/Assets/Exports/UObjectRedirector.cs` | `UE4/Assets/Exports/UObjectRedirector.{h,cpp}` |
| UBlueprintGeneratedClass | `UE4/Objects/Engine/UBlueprintGeneratedClass.cs` | `UE4/Objects/Engine/UBlueprintGeneratedClass.{h,cpp}` |
| UUserDefinedStruct / UUserDefinedEnum | `UE4/Objects/Engine/UUserDefined{Struct,Enum}.cs` | `UE4/Objects/Engine/UUserDefined{Struct,Enum}.{h,cpp}` |
| Reflection flag enums | `UE4/Objects/UObject/{EStructFlags,EClassFlags,EFunctionFlags}.cs` | `UE4/Objects/UObject/{EStructFlags,EClassFlags,EFunctionFlags}.h` + `CoreNetTypes.h` |
| Provider object loading | `FileProvider/AbstractFileProvider.cs` (`LoadPackageObject`) | `FileProvider/IFileProvider.{h,cpp}` (`LoadPackageObject`/`TryLoadPackageObject<T>`) |
| Provider stack | `FileProvider/{AbstractFileProvider,DefaultFileProvider}.cs`, `FileProvider/Vfs/{IVfsFileProvider,AbstractVfsFileProvider,FileProviderDictionary}.cs`, `FileProvider/Objects/{VersionedGameFile,OsGameFile}.cs`, `UE4/VirtualFileSystem/AesVfsReaderForProvider.cs` | same paths under `FileProvider/` and `UE4/VirtualFileSystem/` |
| Virtual paths / plugin manifests | `UE4/Plugins/UPluginManifest.cs`, `FileProvider/AbstractFileProvider.cs` (`LoadVirtualPaths`) | `UE4/Plugins/UPluginManifest.h` (+ `FromJson` per type), `FileProvider/AbstractFileProvider.cpp`, `Utils/Json.{h,cpp}` (no C# counterpart — stands in for Newtonsoft.Json) |
| Localization (`.locres`/`.locmeta`) | `UE4/Localization/{FTextLocalizationResource,FTextLocalizationMetaDataResource}.cs`, `UE4/Objects/Core/i18N/{FTextKey,FEntry,FTextLocalizationResourceString}.cs`, `FileProvider/InternationalizationDictionary.cs`, `FileProvider/AbstractFileProvider.cs` (`ChangeCulture`/`LoadLocalization`/`GetLanguageCode`) | same relative paths (`.h` + `.cpp` each); the culture machinery on `InternationalizationDictionary.{h,cpp}` and the localization surface on `AbstractFileProvider.{h,cpp}` |
| App command layer | `FModel/ViewModels/Commands/{MenuCommand,CopyCommand,RightClickMenuCommand}.cs` | same paths under `FModel/ViewModels/Commands/` (`.h` + `.cpp` each); the `ApplicationViewModel` command properties; `Enums.h` gains `EBulkType`'s `\|`/`&`/`hasFlag` |
| App settings dialog | `FModel/ViewModels/SettingsViewModel.cs`, `FModel/Views/SettingsView.xaml{,.cs}`, `FModel/Views/Resources/Controls/{DictionaryEditor,EndpointEditor}.xaml{,.cs}`, `FModel/Extensions/EnumExtensions.cs` | same relative paths under `FModel/` (`.h` + `.cpp` each, the XAML folded into the `.cpp`); `UE4/Versions/EGame.{h,cpp}` gains `EGameValues()`; `MenuCommand` gains `setOpenWindowHandler` |
| App explorer tree | `FModel/ViewModels/{AssetsFolderViewModel,AssetsListViewModel,GameFileViewModel}.cs`, `FModel/Framework/RangeObservableCollection.cs` | same relative paths under `FModel/` (`.h` + `.cpp` each); `FModel/Framework/CollectionView.h` has no C# counterpart (see the notes above) |
| App game loading | `FModel/ViewModels/{CUE4ParseViewModel,ThreadWorkerViewModel,GameDirectoryViewModel,AesManagerViewModel,GameSelectorViewModel}.cs`, `FModel/ViewModels/Commands/LoadCommand.cs`, `FModel/Framework/{AsyncQueue,FullyObservableCollection}.cs`, `FModel/Helper.cs`, `FModel/Views/{DirectorySelector,AesManager}.xaml{,.cs}` | same relative paths under `FModel/` (`.h` + `.cpp` each, the XAML folded into the `.cpp`); `ApplicationViewModel` gains the provider/AES/worker members + `updateProvider`; `UserSettings` gains `add`/`removePerDirectory` |
| IO Store container | `UE4/IO/IoStoreReader.cs`, `UE4/IO/Objects/{FIoStoreTocResource,FIoStoreTocHeader,FIoStoreTocCompressedBlockEntry,FIoStoreTocEntryMeta,FIoChunkId,FIoOffsetAndLength,FIoDirectoryIndexEntry,FIoFileIndexEntry,FIoStoreEntry,FIoContainerId,FPackageId,FIoStatus}.cs` | same paths under `UE4/IO/` |
| Pixel formats | `UE4/Assets/Exports/Texture/PixelFormat.cs` | `UE4/Assets/Exports/Texture/PixelFormat.{h,cpp}` (`EPixelFormat` 97 members, `FPixelFormatInfo` geometry table, `TryParsePixelFormat` standing in for `Enum.TryParse`) |
| Texture platform data | `UE4/Assets/Exports/Texture/{FTexturePlatformData,FTexture2DMipMap,FVirtualTextureBuiltData,FVirtualTextureDataChunk}.cs` | same paths under `UE4/Assets/Exports/Texture/` |
| Texture export tree (25) | `UE4/Assets/Exports/Texture/{UTexture,UTexture2D,UTextureCube,UTexture2DArray,UVolumeTexture,ULightMapTexture2D,UTextureLightProfile,UTextureProFX,UTextureMovie,UTextureRenderTarget*,UPaperSprite,UTexture*MipDataProviderFactory,…}.cs` | same relative paths, `.h`/`.{h,cpp}` |
| Texture prerequisites | `UE4/Objects/Core/Compression/FCompressedBuffer{,Header}.cs`, `UE4/Assets/Objects/FEditorBulkData.cs`, `UE4/Objects/Engine/UAssetUserData.cs`, `UE4/Assets/Exports/Component/IAssetUserData.cs`, `UE4/Assets/Exports/Material/{CMaterialParams,UUnrealMaterial}.cs` | same relative paths, `.h` |
| Everything not yet ported (1,035) | every remaining `CUE4Parse/**/*.cs` | a placeholder `.h` at the same relative path — `#pragma once`, the right namespace, a `TODO: port …` line, first line `// Stub for CUE4Parse/…`. Included by nothing, compiled into nothing; the tree shape matches the C# source so the gap is browsable. |

## Deliberate differences from C#

These are noted inline in the headers where they occur:

- **No `Stream`/`RandomAccessStream` base.** `Position`/`Length` are plain archive state (this is
  already how `FByteArchive` models them in C#).
- **Generics → templates.** Since template methods can't be virtual, the perf specializations
  (e.g. `FByteArchive::Read<T>`) are hiding overloads on the subclass, as in C#'s `override`s.
  `Read<T>`/`ReadArray<T>` route through the non-template virtual seams `ReadScalar`/`ReadElements`
  so that endian-swapping archives (`FArchiveBigEndian`) affect every inherited helper — the role
  C#'s virtual `Read<T>` plays. A subclass that overrides the byte-source `Read(...)` also adds
  `using FArchive::Read;` to keep the inherited `Read<T>` template visible.
- **`FArchiveBigEndian` swaps per scalar, not per struct.** C# reflects struct field layouts at
  runtime; C++ can't, so composite types must be read field-by-field (each scalar swaps). Scalars
  and primitive arrays swap correctly.
- **Omitted from this reader batch:** `FRandomAccessStreamArchive` (a thin GenericReader wrapper,
  redundant with the file archive).
- **Decompression is a registry with built-in codecs.** C# builds a decompressor from external native
  packages (Oodle, K4os LZ4, ZstdSharp, Zlib-ng); the C++ port routes each algorithm through a registered
  `DecompressFunc` (mirroring C#'s `DecompressorBuilder.Add`) and keeps the same public API (`Decompress`
  overloads, `LOADING_COMPRESSION_CHUNK_SIZE`). What's implemented:
  - **Zlib / Gzip** — a from-scratch DEFLATE inflater (`Compression/Inflate.{h,cpp}`, RFC 1951/1950/1952,
    canonical-Huffman, puff-style). Fully built-in — **no DLL needed** (C# requires the Zlib-ng DLL).
  - **LZ4** — a safe block decoder (`Compression/LZ4.{h,cpp}`), replacing C#'s native P/Invoke.
  - **Oodle / Zstd** — can't be vendored (Oodle is proprietary; a full Zstd decoder is too large), so
    `Compression/{OodleHelper,ZstdHelper}.{h,cpp}` load a native library at runtime (`LoadLibraryA`/`dlopen`)
    and register `OodleLZ_Decompress` / `ZSTD_decompress` — the same runtime-load approach C# uses. Call
    `OodleHelper::Initialize(path)` / `ZstdHelper::Initialize(path)` with a present library.
  - `None` is a plain copy; **Brotli** is unimplemented (C#'s default builder doesn't wire it either) and
    throws `UnknownCompressionMethodException` until registered. LZ4/Zlib/Gzip are auto-registered at load via
    `RegisterBuiltinDecompressors()`. Covered by `tests/test_compression_codecs.cpp` (golden .NET vectors + LZ4).
- **The Core/Math geometry cluster is mutually recursive**, so it is split the same way `FVector`/`FVector4`
  were: each header forward-declares its siblings and *declares* the cross-type members, whose bodies live in
  `Matrix.cpp` / `FQuat.cpp` / `FRotator.cpp` where every type is complete. `FTransform` needs no `.cpp` — by
  the time it is included, everything it touches is already defined. Notes on the cluster:
  - `FMatrix` is a **value type** here (C# makes it a `class`); every use in the source copies or mutates a
    local, so value semantics are faithful, and `FRotationMatrix`/`FQuatRotationMatrix` still derive from it.
  - `FRotator(double,double,double)` is omitted for the same reason as `FVector`'s: alongside the `float`
    overload it makes every all-int construction ambiguous in C++ (C# instead prefers `float`).
  - `FQuat::operator*` uses only the portable scalar path; C#'s SSE `VectorQuaternionMultiply2` fast path
    is not ported.
  - **Normalized results are ~0.17% off unit length**, because `Normalize()` uses the Quake III `InvSqrt`
    approximation — bit-for-bit what C# does. `IsNormalized()` tolerates 0.01, and `tests/test_geometry.cpp`
    compares long vectors with a *relative* tolerance for this reason. This is fidelity, not a defect.
  - Deferred: the `[StructFallback]` `FTransform(FStructFallback)` ctor (the Math layer deliberately does not
    depend on the Assets layer), `TTransform<T>` ctors, and the `System.Numerics` conversions.
- **Core/Math is complete except `UnrealMathSSE`**, which is an SIMD fast path with no semantics of its own —
  every operation it accelerates already exists in the ported scalar form. The bounds cluster that finished it:
  - `FBox`'s `FBox(FVector[] points)` ctor is ported **with its upstream bug intact**: it *sums* the points into
    both `Min` and `Max` instead of taking their extremes. Nothing on the parse path calls it, and a silent
    behaviour change would be a worse surprise than the bug. Every other accumulation path (`operator+`) is
    correct, and `tests/test_bounds.cpp` asserts the property that matters — an accumulated box contains every
    point fed into it.
  - `FBoxSphereBounds::TransformBy` does its arithmetic in `FVector` where C# uses `System.Numerics.Vector3`.
    Every operation involved (component-wise multiply, `Abs`, `Max`, `Dot`) is element-wise and identical in
    both, so the results match; `Vector3.Max` has no `FVector` equivalent and is a private helper.
  - **`FFloat16` gained `ToFloat`/`FromFloat`, which have no C# counterpart.** C# never decodes `FFloat16`
    because it has `System.Half` and uses *that* wherever a half is consumed (`FHalfVector`,
    `FVector3UnsignedShort`). C++ has no standard half, so `FFloat16` doubles as the `Half` stand-in and has to
    carry the conversion. `ToFloat` is exact for all 65 536 encodings (subnormals, infinities and NaNs
    included); `FromFloat` rounds half-to-even like `System.Half`'s narrowing conversion. The cast operator is
    `explicit` — an implicit one would quietly pull `FFloat16` into arithmetic overload resolution everywhere.
  - The `FHalfVector`/`FVector3UnsignedShort` **implicit** conversion operators become explicit `ToFVector()`
    calls: C++ implicit conversions between struct types are far easier to trip over than C#'s, and there are
    only a handful of call sites.
  - `TRangeBound` keeps C#'s `Pack = 1` via `#pragma pack`, so `sizeof(TRangeBound<float>) == 5` and
    `sizeof(TRange<float>) == 10` — the layout they are read with. `tests/test_bounds.cpp` asserts both.
  - `TVector<T>` is backed by `std::vector` rather than `T[]`, which gives it the same value semantics on copy.
- **AES is implemented, not mapped.** C# gets AES from `System.Security.Cryptography` (ECB, `PaddingMode.None`,
  256-bit) and only ever decrypts. C++ has no standard crypto and the port vendors no crypto library, so
  `Aes.cpp` contains the FIPS-197 inverse cipher and key schedule directly. This is the one BCL primitive the
  port reimplements rather than maps, so `tests/test_aes.cpp` pins it to published vectors — the FIPS-197 §C.3
  AES-256 example and the four-block NIST SP 800-38A ECB-AES256 vector — rather than to itself. Encryption is
  deliberately absent: CUE4Parse never encrypts, and an unused encryptor would be untested code sitting next
  to the decryptor. One addition with no C# counterpart, `DecryptInPlace`, saves a copy per compression block
  on the extract path.
- **The pak layer is ported in full except the per-game `GameTypes` hooks.** `FPakInfo`, `FPakEntry` and
  `PakFileReader` cover the legacy, updated, flat and frozen index formats and both extraction paths. What is
  *not* ported is everything reaching into `CUE4Parse.GameTypes` — roughly forty games' bespoke footer
  decryption, extract paths and Lua/ini/csv post-processing (ValorantSource, ArenaBreakout, InZOI, Rennsport,
  GameForPeace, Dragon Quest XI, Century, ProSpi, and the Lua decryptors). Where C# dispatches to one of
  those, the port **throws with the game named** rather than falling through to the generic path: silently
  returning wrong bytes would be far worse than an explicit gap. Every game branch that is self-contained
  arithmetic — the XORs, offset fix-ups, bit-shuffles and magic checks — *is* ported as-is.
- **`FByteBulkDataHeader` is not ported yet, so the partial-read parameter is dropped** from
  `GameFile::Read`/`CreateReader` and `IVfsReader::Extract`. `PakFileReader::Extract` keeps the offset/size
  arithmetic intact around a zero offset, so restoring the parameter with the bulk-data layer is a local change.
- **`FMemoryImageArchive` syncs Position instead of overriding it.** C# overrides the `Position` property to
  forward to the inner archive; `Position` is a plain field on the C++ `FArchive`, so every byte-source seam
  (`Read`/`Seek`/`Serialize`/`ReadScalar`/`ReadElements`/`ReadBytes`/`ReadSpan`) pushes it into the inner
  reader and reads it back. All the templated helpers route through those seams, so the two stay in lockstep
  without a virtual property. Its `ReadMaterialParameterType`/`ReadMaterialUniformPreshaderHeader` and the
  `PointerTable` field wait on the material layer; nothing on the pak path touches them.
- **`AbstractVfsReader::IsValidIndex` range-checks every read, which C# does not need to.** The probe runs on
  data decrypted with a key that is probably wrong, so the length it reads is arbitrary and the seek routinely
  lands outside the buffer. C# gets a bounds exception there; the C++ memory archives `memcpy` without
  checking, so an unchecked probe would read out of bounds instead. It also keeps C#'s wide-string seek
  arithmetic verbatim even though that overshoots the terminator by two code units — changing it would change
  which AES keys are accepted.
- **`Utils/StringComparer.h` has no C# counterpart**: it stands in for `System.StringComparer`, which the VFS
  layer threads through `Mount(StringComparer)` to decide whether pak paths compare case-sensitively. It is
  modelled as a stateful `std::map` comparator, which is what an ordered C++ map needs.
- **`Files` maps to `std::map<std::string, std::shared_ptr<GameFile>, StringComparer>`.** `shared_ptr` is not
  incidental: the updated pak index deliberately aliases one non-encoded entry under several paths, and the
  last write wins on its `Path` — a C# behaviour the port has to keep.
- **`FileProviderDictionary` copies each mounted index instead of referencing it.** C#'s `ConcurrentBag` holds
  references to each reader's live dictionary and the GC pins them; nothing does in C++, so `AddFiles` copies
  the map (cheap — the entries are `shared_ptr` and stay shared with the reader). Lookup order is C#'s
  descending-readOrder scan, with a deterministic tie-break (earliest-added index wins) inside behaviour C#
  leaves unspecified.
- **The provider owns loaded packages.** C#'s `LoadPackage` returns a fresh GC-managed package per call whose
  archives the GC keeps alive; the C++ `Package` requires its archives to outlive it, so
  `AbstractFileProvider` caches `{uasset archive, uexp archive, package}` by game-file path and hands out
  non-owning pointers valid for the provider's lifetime.
- **`AbstractVfsFileProvider` starts with `CustomEncryption` null for every game.** C#'s constructor wires
  ~30 per-game decryptors from `CUE4Parse.GameTypes` (all unported). This is an explicit gap, not silent
  corruption: an encrypted archive for such a game fails the mount-point probe and never mounts, rather than
  decrypting to wrong bytes. Mount/SubmitKeys also run serially — C# fans them out on `Task.Run`; the port
  has no threading layer. `VerifyGlobalData`/`GlobalData`, `FilesById`, `LoadPackage(FPackageId)`,
  `TryFindStoreEntry`, `ScanForPackageRefs`, `LoadIniConfigs`, `LoadVirtualPaths` and `PostMount` ARE
  ported. Deferred with its layer: localization beyond the lookup table (`.locres`).
- **There is no `Newtonsoft.Json`, so `Utils/Json.h` stands in for it.** C# reaches for
  `JsonConvert.DeserializeObject<T>` and gets both a parser and reflective object mapping; C++ has neither.
  `Utils/Json.h` is the parser half — RFC 8259, UTF-8, `\uXXXX` escapes with surrogate pairs, BOM-tolerant,
  read-only — and each ported type writes its own `FromJson` for the mapping half. `LoadVirtualPaths` is its
  first consumer. C#'s `Parallel.ForEach` prefilter over `Files` becomes a plain scan (no threading layer),
  which also makes the duplicate-key winner deterministic instead of bag-order dependent.
- **The config-ini layer is ported, parser included.** C# gets `ConfigIni` from the `Infrablack.UE4Config`
  NuGet package; there is no such package for C++, so the read path it actually uses is vendored under
  `UE4Config/Parsing/` with the same namespace and type names — sections, the token hierarchy
  (comment/whitespace/text/instruction), the five instruction kinds (`Key=`, `+`, `.`, `-`, `!`), per-line
  line-ending detection and `FindPropertyInstructions`. Its behaviour was verified token-by-token against the
  real 0.7.2.97 assembly, and `test_config_ini` encodes that as ground truth (down to `#` *not* being a
  comment marker, `!Key=Val` taking the whole remainder as its key, and an indented `+Key=1` demoting to a
  plain `Set`).
  - **One piece of the Evaluation layer is vendored too**, because both audio providers read their settings
    through it: `PropertyEvaluator` reduces a property's instructions to the values in effect. The five
    kinds are not symmetrical and the differences matter — `Set` replaces the whole list, `Add` (`+`) appends
    only when the value is *not already present*, `AddForce` (`.`) appends unconditionally, `Remove` (`-`)
    erases **every** occurrence rather than the first, and `RemoveAll` (`!`) clears the list and ignores
    whatever value follows it. All comparisons are ordinal, so `+Key=A` does not collide with an existing
    `a`. Each of those was confirmed by calling `PropertyEvaluator.Default.ExecutePropertyInstructions` on
    the real assembly with constructed tokens, and `test_config_ini` pins every one.
  On top of it: `CustomConfigIni DefaultGame`/`DefaultEngine`, `LoadIniConfigs` (which also
  seeds `InternationalizationDictionary::InitFromIni` and `DefaultLightUnit`), `GameDisplayName`
  (NSLOCTEXT/INVTEXT unwrapping with the `ProjectName` fallback) and `AbstractVfsFileProvider::PostMount`,
  which unmounts the archives whose AES key turned out to be wrong. One arm is deferred: a display name that
  indirects through `LOCTABLE(...)` needs `UStringTable` object loading. `TODO` at that site.
- **Localization is ported end to end: `.locres`, `.locmeta`, and the culture machinery over them.**
  `FTextLocalizationResource` handles all three format versions off one code path — `Legacy` (no magic
  number, so the reader rewinds to 0 and takes strings inline), `Compact` (a string lookup table at a
  trailing offset) and `Optimized_CRC32` (pre-hashed namespaces/keys, an entries count to skip, and
  RefCount-decrementing "string stealing"). `FTextLocalizationMetaDataResource` reads the sibling `.locmeta`.
  Notes on the port:
  - **`Entries` is a vector, not a map, on purpose.** C# declares
    `Dictionary<FTextKey, Dictionary<FTextKey, FEntry>>`, but `FTextKey` overrides neither `Equals` nor
    `GetHashCode`, so those dictionaries key on *reference* identity: nothing is ever deduplicated or
    overwritten, and enumeration comes back in insertion order. Insertion-ordered vectors reproduce that
    exactly; a `std::map` keyed on `Str` would silently merge repeated namespaces and reorder them.
  - **`CompiledCultures` gains a `bHasCompiledCultures` flag**, because C#'s `string[]?` distinguishes null
    (below `AddedCompiledCultures`) from empty, and `InitFromMeta` returns early on null.
  - **One bounds check is stricter than C#.** Upstream's guard is `localizedStringArray.Length > index`,
    which lets a *negative* index through into an `IndexOutOfRangeException`; the port adds `index >= 0` so
    it lands in the same "this entry will have no translation" path instead. No shipped file hits it.
  - The `.locmeta` "too new" message interpolates `ELocResVersion.Latest`, not `ELocMetaVersion`'s — an
    upstream copy-paste slip, kept verbatim.
  - `InternationalizationDictionary` now carries `InitFromMeta`, `TryGetCulture` (where a resolvable
    `CultureMappings` alias *overwrites* a direct hit — upstream has no `else`) and `ChangeCulture`, whose
    `.locres` sweep is C#'s `LoadByPattern`. `Parallel.ForEach` becomes a serial scan, which also removes
    the only remaining reason for C#'s `_provider` field.
  - On the provider: `LoadLocalization`, `ChangeCulture`, `TryChangeCulture` and `GetLanguageCode` — the
    per-game culture-code tables (Fortnite, World Explorers, ARK, State of Decay 2, Borderlands 3,
    MultiVersus, Aion 2, then a generic fallback), each with its own default, which is why an unshipped
    language resolves differently per game.
  - Four game-specific arms are deferred with a named `TODO` at each site, all blocked on classes that are
    still stubs: the CodeVein2 and EmbersofTheUncrowned encrypted string tables, the NevernessToEverness
    string table, and `ChangeCulture`'s Aion2 `L10NString.dat` branch. They fall through to the standard
    read rather than throwing.
  - `test_localization` builds a `.locres` per format version and a `.locmeta` per version by hand, and the
    whole path is verified on retail Satisfactory: 224 `.locres` / 9 `.locmeta` found, `fr` loading 8,925
    entries across 55 namespaces, of which 8,406 differ from the `de` table.
- **`VersionContainer` carries its two per-game lookup tables.** `InitOptions` builds the 20 named booleans
  the readers branch on (engine-version thresholds plus the per-game carve-outs — `GAME_DeltaForce` and
  `GAME_ArenaBreakoutMobile` for `RawIndexBuffer`, `GAME_UE4_25_Plus` for skeletal ray-tracing data,
  `GAME_Back4Blood` for `bVisibleInRayTracing`, `GAME_GearsOfWar4`/`GAME_TEKKEN7` for nav collision, and the
  five games that ship 4.25+ sound waves without audio streaming), and `InitMapStructTypes` builds the
  property-name → key/value struct type table. Both are rebuilt whenever `Game` or `Platform` is reassigned,
  with the constructor's override maps re-applied on top — the shape `AppSettings.json`'s per-game
  `"Versioning": { CustomVersions, Options, MapStructTypes }` block feeds. Three readers consume them:
  `ByteProperty`'s MAP-mode width, `FScriptStruct`'s `Vector_NetQuantize*` arms and `UScriptMap`'s key/value
  type substitution. Deliberate differences: the indexer getter throws on an unknown option like C#'s
  `Dictionary` rather than defaulting to false; `MapStructTypes` values are a pair of strings, with `""`
  standing in for C#'s null (only `IsNullOrEmpty` is ever tested); the override tables are plain maps, since
  C# only iterates them; and `ICloneable.Clone()` is dropped in favour of the implicit copy constructor.
- **The whole custom-version family is ported — 40 headers plus the lookup they sit on.** `VersionUtils`'
  `CustomVer(Ar, guid)` answers in C#'s precedence order: the provider's override table (`Versions.CustomVersions`,
  the `AppSettings.json` `"Versioning"` block) first, then the owning package's summary when it is versioned,
  and otherwise `-1` meaning "guess from the game". Each `FXxxObjectVersion` / `FXxxCustomVersion` then keeps
  its own `enum Type`, its `GUID` and a `Get(FArchive&)` that either returns the resolved version or walks the
  per-game ladder. Deliberate differences: C#'s `static class` becomes a namespace (so `FXxx::Type::Member`
  and `FXxx::Member` both resolve, the enum staying unscoped as in C#), and the extension method
  `Ar.CustomVer(key)` becomes the free function `CustomVer(Ar, key)`. Two files keep their C# oddities rather
  than being normalised: `FExternalPhysicsCustomObjectVersion` lives in `UE4/Versions/` but declares itself in
  `CUE4Parse::UE4::Objects::UObject`, exactly as the C# does; and `FAssetRegistryVersion` is not a member of
  the family at all — a standalone `FAssetRegistryVersionType` enum plus a `TrySerializeVersion` that reads a
  leading GUID off the archive and only trusts the following version int when it matches. That enum is scoped
  (`enum class : int32_t`) to keep ~25 member names out of the namespace; every C# call site already spells it
  qualified. `IPackage` grew a `GetSummary()` accessor so `CustomVer` can reach `bUnversioned` and the
  package's own custom-version table.
- **Every enum-only file in the tree is ported — 113 headers across two bulk passes.** The first pass took
  the three enum directories whole (`UE4/Wwise/Enums` 36, `UE4/Wwise/Enums/Flags` 15, `UE4/FMod/Enums` 16);
  the second swept up the 46 remaining C# files anywhere in the tree that declare only enums (`Kismet`,
  `RigVM`, `RHI`, `i18N`, `Texture`, `Material`, `Animation`, `WorldPartition`, `VirtualFileCache`, …).
  Both were generated by a shape-asserting script that refuses anything it does not fully recognise rather
  than guessing; the refusals are what found the outliers below. Conventions and deliberate differences:
  - C# enums are strongly typed, so these are `enum class` with the C# underlying type preserved exactly —
    these values are read straight off the wire and a widened type would consume the wrong number of bytes.
  - A `[Flags]` enum gets the bitwise operators C# grants for free (`| & ~ |= &=`) plus a `HasFlag` free
    function, matching the existing `EUnluacFlags` precedent. `operator~` truncates back to the underlying
    type, so `~` on a byte-backed enum stays inside the byte instead of promoting to `int`.
  - `[Description("…")]` is **runtime data, not decoration** — `FModel/Creator/Utils.cs` calls
    `GetDescription()` and uses the result as an internationalisation lookup key — so it is carried over as a
    generated `Description(E)`, joining the existing per-enum overload set (`ELanguage`, `EMaterialFormat`,
    `ENaniteMeshFormat`), and returning `nullptr` for a member that has none (C#'s extension falls back to the
    member name, which callers must now do themselves). `ETexturePlatform` gained the overload it was missing.
  - `NameOf(E)` (C#'s `ToString()`) is generated **only** where an extension method actually observes the
    member name, which is `EAKBKHircType`, `EAKBKHircType_v125`, `EAkActionType` and
    `EEventActionType_v72_to_v150`. Elsewhere it would be dead code. Enums whose C# carries
    `[JsonConverter(typeof(StringEnumConverter))]` are commented to that effect so the requirement is not lost
    when the JSON writer lands.
  - The three C# `…Extensions` classes are hand-written, since they are logic rather than data:
    `EAKBKHircType`'s `MapToCurrent`/`ToVersionString`, `EAkActionType`'s `ToVersionString`, and
    `EBankSourceFlags`' `MapToCurrent`. These exist because the version pairs are *not* a straight cast of one
    another — v125 lists `FeedbackBus`/`FeedbackNode` inline at `0x10`/`0x11` while the current enum moved
    them to `0x80`/`0x81` and reused `0x10` for `FxShareSet`, and v112's `HasSource` is bit 1 where the
    current enum puts it at bit 7. C#'s extension methods become free functions taking the value first, and
    the ones returning `string` return `const char*` (null for an undeclared value, where C#'s `ToString()`
    would render the number).
  - Faithfulness over tidiness, as with the version family: `EManagedArrayType.cs` sits in
    `UE4/Objects/Chaos/GeometryCollection` but declares `CUE4Parse.UE4.Chaos.GeometryCollection` — no
    `Objects` — and the port keeps the mismatch. `EPlaylistFlags.cs` declares `EPlayListFlags` (capital L) and
    `EAkCompanyID.cs` declares `AkCompanyID`; both keep the C# spelling rather than the file name.
  - Two C# syntaxes have no C++ equivalent and are rewritten: binary literals with `_` digit separators
    (`0b0110_0000` → `0b0110'0000`), and a member initialised from another enum's member
    (`ERHIZBuffer.IsInverted != 0 ? … : …` → an explicit `static_cast`). That RHI ternary is **kept rather
    than folded** even though `IsInverted` is hardcoded to 0 in CUE4Parse, so that the four depth comparisons
    follow automatically if it is ever corrected to UE's real `FarPlane < NearPlane`.
  - C#'s `#pragma warning disable CA1069` marks *intentional* duplicate values (`EAkCurveInterpolation`,
    `EAkPropID`, `EAkBuiltInParam`). C++ allows duplicates silently, so the risk runs the other way and the
    aliases are pinned by test; where a generated `NameOf`/`DescriptionOf` switch would have had two cases at
    one value, the later member is emitted as a comment naming the winner, mirroring .NET.
- **The IO Store layer is ported end to end — container *and* Zen assets.** `IoStoreReader` mounts,
  resolves chunks (perfect-hash with overflow fallback included, comparing `GetHashCode`s exactly as C# does),
  splits partitioned `.ucas` sets and extracts through the block loop; `ContainerHeader()` is a real lazy
  accessor over the container-header chunk, `IoGlobalData` reads the global name batch + script objects off
  `global.utoc`, and `IoPackage` reads a Zen package (both the UE5 `FZenPackageSummary` and the UE4.26-4.27
  `FPackageSummary` paths) and deserializes its exports. The TheFinals/ArcRaiders and NFS Mobile toc
  obfuscation IS ported (self-contained hardcoded-key AES, not GameTypes); eBaseballProSpirit extraction
  throws (needs GameTypes' trailer arithmetic).
  - **Verified end to end on a retail game.** A **Satisfactory 1.2.0 (UE 5.6)** install — 2 containers,
    48,616 files — reads completely: with an Oodle library handed to `OodleHelper::Initialize`, all
    **23,699 `.uasset` files extract and parse**, and 110,664 export objects construct.
  - **A misdeclared `EGame` is the failure mode to rule out first, because it does not look like one.**
    An earlier run of this same install was read as UE 5.3/5.4 (a stale `AppSettings.json` entry from before
    the game updated), which put `Ar.Ver` below `VERSE_CELLS` and so skipped the 8-byte
    `FZenPackageCellOffsets` block those packages carry. The name batch was then read out of the cell
    offsets, its count came out as ~10^8, and the parse died with *"Read size is bigger than remaining
    archive length"* — which reads like a container or block-loop fault, and was recorded as one for a
    while. Both sides of that gate are now pinned by `tests/test_zen_package.cpp`, which builds the same
    fixture package in UE5.0 and UE5.6 shapes and asserts the UE5.6 one *fails* when read at UE5.5.
- **Unversioned properties + the `.usmap` mappings provider are ported.** `UsmapParser` reads the whole
  container (magic/version, optional package versioning, Oodle/Brotli/Zstd compression, name LUT, enums,
  structs) into a `TypeMappings`; `AbstractFileProvider::MappingsContainer` feeds it to
  `IPackage::Mappings()`, and `UObject::DeserializePropertiesUnversioned` drives `FUnversionedHeader` +
  `FIterator` over it — zero-masked values produce a `ReadType::ZERO` tag that reads no bytes. Deliberate
  differences: `TypeMappings` is `shared_ptr`-owned (a `Struct` holds a back-pointer to it, so its address
  must be stable); C#'s `Lazy<Struct?> Super` becomes a cached `Super()` resolve; `PropertyType`/`PropertyInfo`
  are `shared_ptr` so a cloned array-element `PropertyInfo` shares its descriptor exactly as C# does.
  - **Cross-validated against 18 real `.usmap` files** spanning UE 4.20 → 5.7 and usmap versions 0/1/3/4,
    with an independent reference parser written from the format rather than from this code. The two agree on
    every file: 15 parse (matching type/enum counts once C#'s last-wins `structs[s.Name] = s` dedupe is
    accounted for) and the same 3 fail at the same byte. Those 3 are not port defects — they come from
    dumpers whose format CUE4Parse does not implement. The clearest is Satisfactory's shipped
    `CommunityResources/FactoryGame.usmap`: it writes `OptionalProperty` with **no inner type**, where every
    other file (and both CUE4Parse and this port) recurse into one, and it carries 784 KB of trailing data
    after the struct table. Reading it would mean guessing at a foreign layout, so the port fails it exactly
    where the C# does.
- **`IoPackage` deliberate differences.** C#'s `AbstractUePackage : UObject` base is not ported, so
  `IoPackage` implements `IPackage` directly (like `Package`) and repeats the small
  `ConstructObject`/`DeserializeObject` helpers. Export loading is lazy only: the export-bundle walk records
  each export's `(position, newPos)` and `GetExportObject(i)` constructs + deserializes on first use.
  `ImportedPackagesAllVersions` (C#'s "search all previous versions" fallback in `ResolveObjectIndex`) needs
  `IFileProvider.TryLoadPackages`, which is not ported, so a package import resolves only against the
  directly imported package. `ubulk`/`uptnl` payloads are not attached (`FAssetArchive`'s payload subsystem
  is still deferred). `ResolvedScriptObject::Object` IS ported (a cached `UScriptClass`) because unversioned
  deserialization resolves an export's class through it; its `Class` stays null (`ResolvedLoadedObject` is
  unported). C# reads the import/export maps off the *base* archive after seeking the asset archive — which
  only works because C#'s `FAssetArchive` delegates `Position` to its base — so this port reads both through
  the asset archive.
- **Some Utils are partial ports.** `MathUtils` ports the scalar helpers only (the `FVector`/`FQuat`
  conversions and `CubicCurve2D` wait on the Core/Math layer). `StringUtils` ports the ordinal
  substring helpers (culture-aware `StringComparison` overloads and the `Span<char>` overload have
  no `std::string` equivalent; `ParseAesKey` waits on the Encryption layer). `CRC32` omits the
  `Stream`-based `CrcCalculatorStream`. `ArrayUtils` maps `BitArray` to `std::vector<bool>`.
  `EnumUtils`/`DictUtils`/`ListUtils` rely on C# runtime reflection / LINQ and are skipped.
- **Strings are UTF-8 `std::string`.** `ReadFString` converts UCS2 (UTF-16LE) and Latin-1 to UTF-8.
- **Core/Misc value structs derive from an empty `IUStruct` base.** The empty-base optimization keeps
  `FGuid`/`FDateTime`/`FFrameNumber`/`FFrameRate` trivially copyable and standard-layout (16-, 8-,
  4- and 8-byte POD respectively), so they read directly via `FArchive::Read<T>()` as in C#.
- **`FDateTime::ToString()` differs deliberately.** C# formats via `new DateTime(Ticks):F`, a
  culture-dependent long date. That is not reproducible without .NET's culture machinery, so the
  port emits a fixed, deterministic UTC `yyyy.MM.dd-HH.mm.ss` string computed from the tick count.
- **`FGuid` string formats are all ported** (incl. `Short` url-safe base64 and `Base36Encoded` via
  `FUInt128`). `FSHAHash`'s `FIoChunkHash` conversion waits on the IO layer.
- **`FPackageFileSummary` is ported, with two documented gaps.** The out-of-range file-version
  `Log.Warning` is dropped (no logging framework; the condition is noted at its site), and the
  `PACKAGE_FILE_TAG_ONE`/`FAssetArchive.AbsoluteOffset` fix-up is deferred until the asset-reader
  layer (`FAssetArchive`) exists. `VersionContainer` gained an optional `CustomVersions` table
  (a forward-declared `shared_ptr`, so it stays include-cycle-free with `FArchive`).
- **`SerializeCompressedNew` + `FArchiveLoadCompressedProxy` are now ported** (the long-standing
  deferral is closed). `FArchive` gained the nested `FCompressedChunkInfo` reader struct; the
  bulk-data header parsing (v1/v2 tags, byte-swap detection, per-chunk decompress) mirrors the C#.
  Two deliberate differences: `Enum.TryParse<CompressionMethod>` became an explicit name→enum helper,
  and the proxy's `Length` (which *throws* in C#) is modelled as `INT64_MAX` here, since `Length` is a
  plain field feeding `CheckReadSize` and the decompressed size isn't known up front. Actual codecs
  still route through the `Compression` registry — `None` works out of the box; others need a
  registered decompressor.
- **The FName name-pool is ported** (`FName` is now the full struct, not the text-only subset). `FName`
  stores an `FNameEntrySerialized` and gains the name-map / `FMappedName` index constructors alongside the
  string path; `==` uses index equality on the Index path and ASCII `OrdinalIgnoreCase` text folding on the
  Text path (the .NET culture-invariant Unicode fold is not reproduced — names are ASCII in practice).
  `FNameEntrySerialized` ports both the classic per-package FString entry and the IO Store name-batch
  loaders (`LoadNameBatch` + `FSerializedNameHeader`). Deliberate differences there: `NAME_HASHES` is not
  defined so the two hash `ushort`s are skipped (the C# `#else` path), the PlayerUnknownsBattlegrounds name
  remap (an embedded `PUBGNameHashMap.json`) is not vendored and is skipped with a TODO, `Name.Trim()` trims
  ASCII whitespace only, and the batch string decode always treats UTF-16 as little-endian (as `ReadFString`
  does) rather than routing char reads through the archive's endianness.
- **The import/export map layer is ported** (`FAssetArchive`, `FPackageIndex`, `FObjectImport`,
  `FObjectExport`) on top of a minimal `IPackage` (just `NameMap()` + `HasFlags()`). Deliberate
  differences: C# delegates `FAssetArchive`'s `Position`/`Length` to the base archive as *properties*;
  this port keeps them as plain archive state and treats the base as a random-access byte store addressed
  by this archive's `Position` (so `Ar.Position += n` skips still work) — a consequence is that typed reads
  go through `FAssetArchive` (little-endian), not the base's `Read<T>`. Object resolution/loading is
  deferred: `FPackageIndex::Name()` returns `"None"`, so `FObjectExport::ClassName` /
  `ToString()`'s class part are `"None"` until resolution exists. Also deferred with the asset-export
  layer: `FObjectExport::GetPublicExportHash`/`GetGlobalImportIndex`, the `FKismetArchive` `FPackageIndex`
  ctor, and `FAssetArchive`'s payload subsystem (`GetPayload`/`ReadObject<T>`). All marked `TODO`.
- **The classic-package reader is ported** (`Package` reads the `FPackageFileSummary` + name/import/export
  maps from a `.uasset` and implements `IPackage`), together with index resolution: `ResolvedObject`
  (name + Outer/Class/Super chain + `GetPathName`/`GetFullName`) and a real
  `IPackage::ResolvePackageIndex`, so `FPackageIndex::Name()`, `FObjectExport::ClassName`, and
  `FObjectExport::ToString()` now resolve for real. Deliberate differences: C#'s `Package` derives from
  `UObject` — this port implements `IPackage` directly (no `UObject` yet); the optional summary tables
  (thumbnails, depends, preload deps,
  soft-object paths, data resources, trailer) are not read yet; byte-swapped (big-endian) packages throw for
  now; and `FPackageIndex`'s `WeakReference` name cache is not reproduced (Name/ToString re-resolve). All
  marked `TODO`.
- **A minimal `IFileProvider` is ported, so cross-package imports resolve for real.** `IFileProvider`
  (`FileProvider/IFileProvider.h`) exposes the surface the reader layer consumes — `GetVersions`,
  `GetInternationalization`, and `TryLoadPackage` (returns a provider-owned `IPackage*`, null on miss) — plus
  `InternationalizationDictionary` (the namespace→key→string localization table: `SafeGet`/`Override`). A
  `Package` now carries its provider, and `ResolveImport` walks an import to its outermost package, loads that
  package through the provider, and matches the import by object name + resolved outer path — so an import
  resolves to the actual export in the other package (and `ResolvedObject::Load` returns that package's
  object). `FTextHistory::Base` now resolves its `LocalizedString` through the provider's `Internationalization`
  table (as in C#). Deliberate differences: the rest of C#'s large `IFileProvider` (the `Files`/`GameFile`
  VFS, virtual/texture-cache paths, config inis, mappings container, the SaveAsset/CreateReader/SavePackage/
  SaveAsset/CreateReader/SavePackage families) arrives with its layers; `TryLoadPackage` returns a `Package*`,
  callers hold non-owning pointers (the provider owns loaded packages); the IoPackage import branch,
  `/Script/` packages, and misses fall back to the in-package `ResolvedImportObject`. All marked `TODO`.
  (`.locres` loading and the culture machinery used to be deferred here, so the table had to be populated by
  hand through `Override`; both are ported now — see the localization section below.) **`LoadPackageObject`/`TryLoadPackageObject<T>` are now ported** (in
  `IFileProvider.cpp`): they split a path into (package, object) like C#'s `GetPathName`, load the package, and
  return the named export cast to `T*` — used by `StringTableEntry` below.
- **Concrete export types build through an `ObjectTypeRegistry`.** C# discovers
  `UObject` subclasses by reflection; C++ has none, so `ObjectTypeRegistry` (`UE4/Assets/ObjectTypeRegistry.{h,cpp}`)
  keeps a manual name→factory map (each type registers under its serialized class name, e.g. `StringTable`, `DataTable`).
  `Package::ConstructObject` now consults the registry with the resolved class object's name — the same string
  C#'s `UScriptClass.ConstructObject` ultimately looks up — so a `StringTable` export deserializes into a
  `UStringTable` (its `FStringTable`: namespace + key→string entries, plus the metadata map) and a `DataTable`
  into a `UDataTable` instead of a bare `UObject`. Unregistered classes still fall through to base `UObject`.
  - **The name is the whole risk.** C# derives the registered name *from the type* (`RegisterClass(Type)` strips
    a leading `U`/`A`), so name and type can never disagree. Here they are an independently written string and
    type; a typo compiles fine and silently costs every typed field. A `Register<T>(name)` helper keeps the pair
    on one line, and `tests/test_object_type_registry.cpp` builds every registered name and asserts
    `typeid(*obj) == typeid(T)` — exact type, not a base, since most of these differ from their base only in
    which fields exist.
  - **77 types are registered**, the 13 engine ones plus all 64 concrete audio types: `Sound` (7 —
    `USoundBase` is `abstract` in C#, so reflection skips it and so do we), `Sound/Node` (20), `MetaSound` (2),
    `Wwise` (28, including the per-game renames `UWuiBank`/`UWwiseBank` and `UWuiEvent`/`UWwiseEvent`, whose
    only reason to exist is a different serialized class name) and `FMod` (7). The one deliberate oddity:
    `Exports/Sound/UMetaSoundSource.cs` declares its class as **`UMetaUSoundSource`** — a typo in the C# that
    reflection turns into the registered name `MetaUSoundSource`, which no cooked asset uses. The real
    MetaSound source is a *different* type registered as `MetaSoundSource`. Both are kept verbatim; "fixing"
    the name would silently change which type a package constructs.
  - **What this buys on real data.** Satisfactory (UE 5.6): 2,668 more exports construct as typed objects —
    2,502 `UAkAudioEvent`, plus `UAkStateValue`/`UAkRtpc`/`UAkSwitchValue`/`UAkAcousticTexture`/`UAkAuxBus`/
    `UAkInitBank` — 24,494 of 110,664 (22.1%) typed overall. Poppy Playtime Chapter 5 (UE 5.6, with readable
    mappings): 3,598 typed audio exports, and **all 3,000 `USoundWave`s read a valid `CompressedDataGuid` and
    a non-empty `FStreamedAudioPlatformData`** (`BINKA`, a 28-byte header chunk + the payload chunk), while the
    566 `UMetaSoundSource`s carry no wave payload — `SoundBaseDeserialize` opting out, exactly as in C#.
  - **Registration is necessary but not sufficient.** A typed `Deserialize` only reaches its own fields after
    the tagged-property read succeeds, and for a UE5 Zen package that needs mappings. With Satisfactory's
    unreadable `.usmap`, all 110,664 exports read **zero** properties and every registered type stays at its
    defaults. The largest single unregistered class name in that game is `Texture2D` (6,127 exports).
- **`UDataTable`** (`UE4/Assets/Exports/Engine/UDataTable.{h,cpp}`) recovers its `RowStructName` from the tagged
  `RowStruct` `ObjectProperty`, then reads its `RowMap` (row name → `FStructFallback`, an insertion-ordered
  `vector<pair<FName, FStructFallback>>`). C# loads the row `UStruct` to drive parsing; that's deferred, so the
  port always takes the equivalent by-name `FStructFallback` path (identical for tagged assets). This is the
  first export to exercise the already-ported `FStructFallback` end-to-end.
- **`UCurveTable`** (`UE4/Assets/Exports/Engine/UCurveTable.{h,cpp}` + `ECurveTableMode.h`) is the same shape as
  `UDataTable`: a `RowMap` (row name → `FStructFallback`, insertion-ordered `vector<pair<FName, FStructFallback>>`),
  but the row struct name comes from the serialized `ECurveTableMode` (`SimpleCurves`→`SimpleCurve`,
  `RichCurves`→`RichCurve`, `Empty`→`""`) rather than a `RowStruct` property. The `bUpgradingCurveTable` branch
  (`FFortniteMainBranchObjectVersion < ShrinkCurveTableSize`, which infers the mode from `numRows`) is
  assume-modern — the mode byte is always read. `FindCurve`/`TryFindCurve` wrap a row in the concrete curve
  (see below).
- **The curve evaluation subsystem is ported** (`UE4/Objects/Engine/Curves/{RealCurve,SimpleCurve,RichCurve}.{h,cpp}`).
  `FRealCurve` is the abstract base (`DefaultValue` + pre/post-infinity extrapolation + `CycleTime`); `FSimpleCurve`
  (single interp mode + `(Time,Value)` keys) and `FRichCurve` (per-key interp mode + tangents, optionally weighted)
  add their key arrays + a faithful `Eval`/`RemapTimeValue` (binary-search key lookup, linear/cubic/constant
  interpolation, and the weighted-tangent bezier path via a ported `CubicCurve2D::SolveCubic`). Because C#'s
  reflection `GetOrDefault` has no port equivalent, the curves are built from an `FStructFallback` by scanning its
  tagged `Properties` (a `Keys` `ArrayProperty` of `StructProperty` -> per-key `FStructFallback`; the enum fields
  accept an `EnumProperty` name or a raw `ByteProperty`). `UCurveTable::FindCurve` uses this to hand back an
  `FSimpleCurve`/`FRichCurve` for the table's mode. **Deferred:** `FCompressedRichCurve` + the
  `AnimCurveCompressionCodec` converter map (the unsafe quantized-key decompressors) and the binary
  `FMutableArchive`-read ctors — the animation-compression arc; curve tables serialize as tagged data. The
  well-known CUE4Parse quirk in `WeightedEvalForTwoKeys` (`sinAngle = Cos(angle)` for the arrive tangent) is
  preserved verbatim. `UnrealMath` (`SmallNumber`/`IsNearlyZero`) + `CubicCurve2D` were added to the math layer to
  support this.
- **The `UCurve*` asset family is ported** (`UE4/Objects/Engine/Curves/{UCurveBase,UCurveFloat,UCurveVector,UCurveLinearColor}.{h,cpp}`) —
  the first real exports built on the curve subsystem. `UCurveBase` is the abstract `UObject` subclass (not
  registered); `UCurveFloat` is an empty subclass (its single curve stays a tagged `StructProperty`);
  `UCurveVector` (3 curves) and `UCurveLinearColor` (4 curves) pull their `FRichCurve`s out of the object's own
  top-level `StructProperty(RichCurve)` values, positionally, into a `std::array<FRichCurve, N>` (default-empty
  slots instead of C#'s nulls; the positional index is bounded to the array size). Registered as `"CurveFloat"`/
  `"CurveVector"`/`"CurveLinearColor"`. `UCurveLinearColor::GetUnadjustedLinearColorValue` /
  `GetLinearColorValue` (the HSV brightness/vibrance/saturation/hue/alpha adjustment pipeline) are now implemented
  on top of the Core/Math `FLinearColor` (below). The adjusted pixel-HSV is computed but discarded before
  `HSVToLinearRGB` — a faithful reproduction of FModel's behaviour, so the returned RGB round-trips the unadjusted
  color and only the alpha (via `Lerp(AdjustMinAlpha, AdjustMaxAlpha, A)`) actually changes.
- **The Core/Math color layer is ported** (`UE4/Objects/Core/Math/{FColor,FLinearColor}`). `FColor` is the 8-bit
  gamma-space color (field order `B,G,R,A` matching the C# sequential layout) with `Hex`/`ToPackedARGB`/
  `Requantize16to8`; `FLinearColor` is the 32-bit float RGBA with `ToFColor(sRGB)`/`ToSRGB`/`Hex` and the
  `LinearRGBToHsv`/`HSVToLinearRGB`/`WithAlpha` conversions. `UnsafePrint::BytesToHex` (uppercase hex) backs the
  `Hex` accessors; `UnrealMath` gained `Min3`/`Max3`/`Fmod`. **Deferred:** the `System.Numerics` implicit
  `Vector3`/`Vector4` conversions (no vector consumer yet) and `FColor::Serialize` (no `Writers` layer yet).
- **The `FField`/`FProperty` reflection system + the `UStruct` family are ported** (`UE4/Objects/UObject/`).
  `FField` (`FField.{h,cpp}`) is the base; `UnrealType.{h,cpp}` holds `FProperty` and its ~35 subclasses
  (`FIntProperty`, `FBoolProperty`, `FStructProperty`, `FArrayProperty`, `FEnumProperty`, the Verse types, …),
  with `FField::Construct` dispatching on the serialized field-type name. On top of that, `UField → UStruct →
  {UScriptStruct, UClass, UFunction}` and `UField → UEnum` deserialize their reflection data (super/children,
  `ChildProperties`, struct/class/function flags, enum name→value pairs, implemented interfaces, the CDO index).
  `UScriptStruct`/`UEnum`/`UFunction` are registered (as `"ScriptStruct"`/`"Enum"`/`"Function"`) so those exports
  build into concrete types via the existing name-keyed `ConstructObject`; `UStruct`/`UClass` are intentionally
  left unregistered, mirroring C#'s `[SkipObjectRegistration]`. Deferred (documented as TODO): the Kismet
  bytecode (`ScriptBytecode` is skipped), `UClass.ConstructObject`/Blueprint decompiler, and the custom-version
  gates (`FFrameworkObjectVersion`/`FCoreObjectVersion`/…) — the readers assume modern layouts.
- **`UObjectRedirector`** (`UE4/Assets/Exports/UObjectRedirector.{h,cpp}`) is a trivial export: base `UObject`
  then a `DestinationObject` `FPackageIndex`. Registered as `"ObjectRedirector"`.
- **`UBlueprintGeneratedClass`** (`UE4/Objects/Engine/UBlueprintGeneratedClass.{h,cpp}`) is the concrete `UClass`
  a Blueprint compiles to — the first registered `UClass` subclass, so a cooked BPGC export (whose class import
  is `"BlueprintGeneratedClass"`) now deserializes fully instead of falling back to a bare `UObject`. Its one
  custom-serialized member, the cooked `EditorTags` map, is ported (assume-modern for the
  `FFortniteMainBranchObjectVersion` gate); the fields C# reads via the reflection accessor `GetOrDefault`
  (`NumReplicatedProperties`, the component/timeline `FPackageIndex` arrays, the construction-script indices)
  are recovered by scanning the base tagged `Properties` list, mirroring `UDataTable`'s `RowStruct` extraction.
  Registering BPGC by name is enough for a native BPGC export; the full `SuperStruct`-chain walk in
  `AbstractUePackage.ConstructObject` (which would matter for an *instance* of a Blueprint, class name `SomeBP_C`)
  stays deferred — it needs to load a class reference as a `UStruct`, and in this port an import does not load to
  a `UStruct`/`UScriptClass`, so the walk would dead-end at a base `UObject` regardless (see `Package::ConstructObject`).
- **`UUserDefinedStruct` / `UUserDefinedEnum`** (`UE4/Objects/Engine/UUserDefined{Struct,Enum}.{h,cpp}`) are the
  Blueprint-authored struct/enum, registered as `"UserDefinedStruct"` / `"UserDefinedEnum"`. `UUserDefinedEnum`
  is a pure `UEnum` subclass (C#'s only extra member is a commented-out `DisplayNameMap`), so the registry just
  produces the concrete type over `UEnum`'s existing name/CppForm/underlying-type deserialization.
  `UUserDefinedStruct` reads its compile `Status` (a tagged `"Status"` `EnumProperty`, recovered by scanning
  `Properties` since this port has no `GetOrDefault` — the enum member is parsed from the `Enum::Member` FName),
  then, when up-to-date and not a CDO, a `uint32` `StructFlags` and the default instance (a tagged property list,
  read via `DeserializePropertiesTagged(..., isStruct:true)`). The `FFrameworkObjectVersion` gate for the default
  instance is assume-modern (the block is present), guarded by a `Position < validPos` check so older assets
  without it don't over-read.
- **The tagged-property system is started** (`UObject` core + the versioned/tagged property path). `UObject`
  reads its properties via `FPropertyTag` (classic pre-UE5 tag layout) → `FPropertyTagData` (type descriptor)
  → `FPropertyTagType::ReadPropertyTagType` (the type→reader factory), then the trailing optional `ObjectGuid`.
  A scalar subset of property types is ported (Bool/Byte/Int8/Int16/Int/Int64/UInt16/UInt32/UInt64/Float/
  Double/Name/Str). The **unversioned** property path (`FUnversionedHeader` + the `.usmap` mappings provider)
  is now ported too — see the IO Store section above. Deliberate differences: the UE5
  `PROPERTY_TAG_COMPLETE_TYPE_NAME` tag layout throws; the
  reflection-driven `GetValue<T>`/`PropertyUtil.GetOrDefault<T>` accessors and the JSON `WriteJson` path are
  omitted (consumers read the `Properties` list and `dynamic_cast` a tag's value); `GenericValue` is dropped.
  The tagged-property value set is now complete except for game-specific readers (Borderlands4/OuterWorlds2/…),
  which return null. All `TODO`.
- **The aggregate property types are started** (`StructProperty`, `ArrayProperty`, `EnumProperty` + their value
  types `FScriptStruct`, `FStructFallback`, `UScriptArray`). A struct value reads recursively as a bag of
  tagged properties (`FStructFallback` → `DeserializePropertiesTagged`); an array reads its element count then
  each element via the same `ReadPropertyTagType` factory, including the classic `INNER_ARRAY_TAG_INFO` inline
  tag that carries a struct array's element type. `FScriptStruct`'s named-struct switch is now ported for
  every struct type the port has (`FVector`/`FGuid`/`FColor`/`FPlane`/`FQuat`/`FRotator`/`FSphere`/`FMatrix`/
  `FSoftObjectPath`/the `TIntVector` family/the curve keys/…); the rest still take the C# `default:`
  `FStructFallback` arm. Because `IUStruct` is deliberately an empty non-virtual marker here (so the value
  structs stay trivially copyable for `FArchive::Read<T>`), it cannot be C#'s polymorphic `StructType` slot:
  the value is boxed in a small `IUStructHolder` and read back with `Get<T>()` / `AsFallback()`. Still
  deferred: the `FMaterialInput`/`FExpressionInput`, MovieScene/Niagara/PCG/StateTree/cloth/instanced-struct
  families, `FGameplayTagContainer`, the `FPerPlatform*` set, and every per-game arm. The
  `Vector_NetQuantize*` arms branch on the `Vector_NetQuantize_AsStruct` option, so a UE5 game reads them as a
  tagged property bag and everything older as a bare `FVector`. `UScriptArray`'s `InnerTypeData` paths (unversioned
  / RAW / pre-UE4 / UE5 complete-type-name) and the DaysGone size heuristic are deferred with their layers.
  `EnumProperty` resolves an index to `"EnumName::index"` (serialized-UEnum and mappings resolution deferred),
  and its unversioned/RAW underlying-numeric path falls back to a single byte. `StructProperty`/`ArrayProperty`
  derive `FPropertyTagType` directly rather than `TPropertyTagType<T>` (no scalar `PropValueToString` for an
  aggregate payload; both override `ToString`). All `TODO`.
- **The container property types are started** (`MapProperty`, `SetProperty`, `ObjectProperty` + their value
  types `UScriptMap`, `UScriptSet`). A map/set reads the leading remove-count (keys/elements read and discarded),
  then its entry/element count, then each via the `ReadPropertyTagType` factory (`MAP`/`ARRAY` read type). An
  object property reads an `FPackageIndex`. Deliberate differences: C#'s `UScriptMap`
  `Dictionary<FPropertyTagType, FPropertyTagType?>` becomes an **ordered vector of `unique_ptr` pairs** —
  `FPropertyTagType` has no C++ hash/equality and the reader only needs insertion order (a null key still falls
  back to a synthetic `StrProperty "UNK_Entry_i"`). `UScriptMap` applies the `Versions.MapStructTypes` table,
  which names the key/value struct type for property names whose tag does not carry one (the MovieScene and
  Niagara maps); C# writes the resolved descriptors back onto the caller's `FPropertyTagData`, but that
  descriptor is shared across every read of the property, so the port applies them locally instead. The
  pre-`PROPERTY_TAG_SET_MAP_SUPPORT` game-specific key/value/inner type inference and the many
  game-specific set `InnerTypeData` inferences are deferred (struct elements still read via
  `FStructFallback`). `ObjectProperty`'s game-specific reader special cases (`FLevelSaveRecordArchive` string
  form, `FOW2ObjectsArchive`) are omitted. All `TODO`.
- **The soft / object-ish property types are started** (`SoftObjectProperty`, `WeakObjectProperty`,
  `LazyObjectProperty`, `ClassProperty`, `AssetObjectProperty` + the value types `FSoftObjectPath`,
  `FUniqueObjectGuid`). `SoftObjectProperty` (also the reader for `SoftClassProperty`/`ReferenceProperty`) wraps
  an `FSoftObjectPath` (asset FName + sub-path FString); `WeakObjectProperty`/`ClassProperty` subclass
  `ObjectProperty` and serialize as an `FPackageIndex`; `LazyObjectProperty` reads an `FUniqueObjectGuid` (one
  `FGuid`); `AssetObjectProperty` (also `AssetClassProperty`) reads a plain FString. Deliberate differences:
  `FSoftObjectPath` ports only the **classic** read path (asset FName + sub-path FString) — the pre-
  `ADDED_SOFT_OBJECT_PATH` string form, the UE5 `ADD_SOFTOBJECTPATH_LIST` package-table index, the
  `FSOFTOBJECTPATH_REMOVE_ASSET_PATH_FNAMES` `FTopLevelAssetPath` form, the `FFortniteMainBranchObjectVersion`
  UTF-8 sub-path gate, and the AshesOfCreation/OuterWorlds2/DragonQuestXI game readers are deferred. The
  **`FSoftObjectPath::Load`/`TryLoad` family is now ported** (sync only): `Load(provider)`/`TryLoad(provider, out)`
  resolve `AssetPathName` through `IFileProvider::LoadPackageObject`, the no-arg `Load()`/`TryLoad(out)` pull the
  provider from `Owner->GetProvider()`, the typed `Load<T>`/`TryLoad<T>` `dynamic_cast` the result, and
  `TryResolveSubObject` walks the `'.'`-separated `SubPathString` via `IPackage::GetExportOrNull` (this is what
  motivated wiring `UObject::Owner`, set in `Package::GetExportObject`). C#'s async `LoadAsync`/`TryLoadAsync`
  have no C++ `Task` equivalent and are omitted; C#'s `Load(provider)` throws on a missing package while this
  port's `LoadPackageObject` returns null, so `Load(provider)` returns null on a miss (typed `Load<T>` still
  throws on a null/wrong-type result). `SoftObjectProperty`/`LazyObjectProperty` derive
  `FPropertyTagType` directly (aggregate-style) and override `ToString`; `ObjectProperty::ToString` now uses a
  virtual `TypeName()` so the `Weak`/`Class` subclasses print their own name (mirroring C#'s `GetType().Name`).
  All `TODO`.
- **The delegate / interface / field-path / string-variant property types are ported** (`DelegateProperty`,
  `MulticastDelegateProperty` + the `Inline`/`Sparse` subclasses, `InterfaceProperty`, `FieldPathProperty`,
  `AnsiStrProperty`, `Utf8StrProperty`, `VerseStringProperty` + the value types `FScriptDelegate`,
  `FMulticastScriptDelegate`, `FScriptInterface`, `FFieldPath`). A delegate is an `FPackageIndex` + function
  `FName`; a multicast is a count-prefixed list of them; an interface is an `FPackageIndex`; a field path is a
  count-prefixed `FName` list + resolved-owner `FPackageIndex`. `AnsiStr`/`Utf8Str`/`VerseString` subclass
  `StrProperty` and only change the read (`ReadFAnsiString`/`ReadFUtf8String`). `VerseFunctionProperty` also maps
  to `DelegateProperty`. Deliberate differences: the delegate/interface/field-path value types derive
  `FPropertyTagType` directly (aggregate-style) and override `ToString`; `MulticastDelegateProperty::ToString`
  routes through a virtual `TypeName()` so the `Inline`/`Sparse` subclasses print their own name. C# gives
  `FScriptDelegate`/`FMulticastScriptDelegate` no `ToString` (they'd print the .NET type name) — the port adds
  readable forms (function name / `[N invocations]`). `FFieldPath`'s owner-serialization read is gated in C# on
  `FFortniteMainBranchObjectVersion`/`FReleaseObjectVersion` custom versions; since custom-version providers
  aren't ported, the port assumes modern assets and always reads it. The `FKismetArchive` `FFieldPath` ctor and
  the JSON `WriteJson` resolution are deferred with their layers. All `TODO`.
- **The Text and Optional property types are ported, closing the tagged-property value set** (`TextProperty`
  + the whole `FText`/`FTextHistory` i18N subsystem, and `OptionalProperty`). `FText` reads its flags + history
  type then one of the 13 `FTextHistory` subclasses (`None`, `Base`, `NamedFormat`, `OrderedFormat`,
  `ArgumentFormat`, `FormatNumber`, `AsDate`, `AsTime`, `AsDateTime`, `Transform`, `StringTableEntry`,
  `TextGenerator`), with the `FFormatArgumentValue` / `FFormatArgumentData` / `FNumberFormattingOptions` helper
  structs. `OptionalProperty` reads a presence bool then (if present) the inner property via the factory with the
  `OPTIONAL` read type. Deliberate differences: `Base` localization now resolves through the provider (see the
  `IFileProvider` bullet); `StringTableEntry` now resolves too — it loads the `UStringTable` at its `TableId`
  via `provider->TryLoadPackageObject<UStringTable>`, takes the table entry for its `Key` as `SourceString`,
  and localizes it through the provider (see the `ObjectTypeRegistry` bullet); the many custom-version gates
  (`FEditorObjectVersion`/`FFortniteMainBranchObjectVersion`/`FUE5ReleaseStreamObjectVersion`) aren't ported, so
  the port takes their **modern** outcome at each site (documented inline); the game-specific quirks
  (Splitgate2/DeltaForce/HonorofKingsWorld/EmbersOfTheUncrowned) are omitted; `FFormatArgumentValue`'s C# boxed
  `object Value` becomes explicit typed fields keyed by `Type`; `TextProperty` derives `FPropertyTagType`
  directly and `OptionalProperty` holds a `unique_ptr` to the inner value (null when absent, matching C#'s
  empty-string `ToString`). `OptionalProperty` passes `nullptr` for the inner tag's `InnerTypeData` (deferred
  with `FPropertyTagData`). All `TODO`.
- **Lazy export object loading is ported** (`Package::GetExportObject` + `ConstructObject`/`DeserializeObject`,
  `IPackage::GetExportObject`, `ResolvedObject::Object`/`Load<T>`). Each export is constructed and deserialized on
  first access and cached in `Package::ExportsLazy` (parallel to `ExportMap`): a base `UObject` is built, its
  `Name`/`Outer`/`Super`/`Template`/`Class` resolved from the export record (`Outer` falling back to the package
  itself), then its serial range is re-read from the export archive (`.uexp`, or the `.uasset` when there is no
  `.uexp`) and its tagged properties deserialized. `ResolvedObject::Object()` returns the package's lazy export,
  so `Load<T>()` (a `dynamic_cast`) and `FPackageIndex` references now resolve to a real object. Deliberate
  differences: only the **lazy** (`useLazySerialization`) path is ported — the dependency-graph `ExportLoader`
  path (`useLazySerialization == false`, walking `PreloadDependencies` through Create/Serialize phases) is not.
  `ConstructObject` is **simplified to always build a base `UObject`** (C#'s `default:` path): the `UStruct`/
  `UClass` traversal that would pick a code-defined class is deferred with those export types. A serialization
  failure is swallowed (C# logs it / rethrows only under `FatalObjectSerializationErrors`; there is no logging
  framework here). The `ubulk`/`uptnl` payloads and the `PostLoad` bodies of concrete exports are deferred. The
  archives passed to `Package` must outlive it (their bytes are re-read lazily). All `TODO`.
- **Namespace-shadowing note:** introducing `CUE4Parse::UE4::Assets::Objects` (for the property types) means a
  bare `Objects::UObject::X` written *inside* `namespace …::Assets` now resolves to `Assets::Objects`, not
  `UE4::Objects`. Such references (in `IPackage`, `FAssetArchive`, `Package`, `ResolvedObject`) are now fully
  qualified as `CUE4Parse::UE4::Objects::UObject::X`.
- **The Wwise soundbank tree is ported — 158 files, everything under `UE4/Wwise` bar three.** `FWwiseArchive`,
  the FNV-1 hash, `WwiseVersionInfo`, the bank/AKPK headers, ~57 bank object structs, the 15 `CAkAction*`
  classes, ~50 plugin parameter sets across nine vendor namespaces behind `WwisePlugin`'s ~75-arm dispatch,
  and the whole HIRC hierarchy (25 container types) with C#'s rewind-and-reparse recovery. Conventions and
  deliberate differences:
  - `FWwiseArchive` **does not share a cursor with the archive it wraps.** C# overrides the `Position`
    property to forward to the inner archive; `Position` is a plain field on this port's `FArchive` (see the
    note at the top of `FArchive.h`), so the wrapper syncs its position into the inner around every `Read`
    and `Seek` — which covers every inherited helper, since they all route through those two virtuals. The
    visible consequence is that `Clone()` yields an **independent** cursor where C#'s shares one.
  - `using FArchive::Read;` in `FWwiseArchive` is load-bearing, not tidiness: overriding the non-template
    `Read(uint8_t*, int, int)` hides the inherited `Read<T>()` template, and `Ar.Read<uint32_t>()` then
    parses `<` as less-than. `FByteArchive` sidesteps this by redeclaring the templates.
  - Two near-identical pairs are kept distinct because they read different widths: `ReadBool()` (one byte,
    Wwise) vs `FArchive::ReadBoolean()` (four), and `Read7BitEncodedIntBE()` — which accumulates
    most-significant-first — vs `FArchive::Read7BitEncodedInt()`. Both pairs have call sites in this tree and
    both are pinned by test (`0x82 0x03` is 259 big-endian, 386 little-).
  - **Two C# file-level cycles are broken by relocating leaf types**, keeping the namespace and type names
    unchanged and documenting the move at both ends: `AkSwitchGraphPoint`/`AkRtpcGraphPoint` move from
    `AkRTPC.h` into `AkConversionTable.h`, and `AkFilterBand` into `CAkParameterEQFXParams.h`.
  - C#'s explicit-layout union in `AkPropBundle` becomes an `AkUnionValue` holding the raw `uint32_t` with
    `f32()`/`IsFloat()` accessors — the same magnitude guess C# makes, made explicit. `HierarchyEventAction`'s
    `object?` payload becomes a type-erased `unique_ptr<IActionData>` plus a `Get<T>()` that returns null for
    the wrong alternative. `AkDecisionTree` children are `unique_ptr` (the C# tree is reference-typed).
  - `Enums::MapToCurrent(rawType, version)` must be **fully qualified** at its `Hierarchy` call site: both
    arguments are plain integers, so ADL cannot reach the `Enums` namespace. The enum-argument overloads
    (`MapToCurrent(EBankSourceFlags_v112)`, `HasFlag`) resolve normally.
  - Every blitted struct carries `#pragma pack` and a `static_assert(sizeof(...))`, and every faithful quirk
    is commented rather than "fixed" — the fourth unused byte per `AkFxChunk`, `CAkReflectFXParams`' curves
    filled out of order by an embedded index, `DSPLfoWaveform`'s intentional CA1069 duplicate, the
    `-42.0f` sentinel gating `CMBCRuntimeParams`' mode, `AkFolder`'s UTF-16 name (C#'s `char` is two bytes),
    and `CREVSourceModelPlayerParams`' hand-rolled big-endian float read.
  - `WwiseReader`, `WwiseProvider` and `Objects/AkEntry` came in later slices, once the `FDeferredByteData`
    family they need existed. `UE4/Wwise` is now complete.
- **The bulk-data / payload layer is ported, and with it the Wwise container reader.** `TBulkData<T>` +
  `FByteBulkData` read a payload that lives out of line from the export that owns it; `FByteBulkDataHeader`
  resolves where that is, three different ways (an index into a UE5 `IoPackage`'s `BulkDataMap`, an index
  into a classic `Package`'s `DataResourceMap`, or an inline header — the two map arms give their 4 index
  bytes back when the lookup misses). `FAssetArchive` regained its payload registry, and the optional
  `FByteBulkDataHeader*` that narrows a read to one sub-range is threaded back through `GameFile::Read` and
  both `IVfsReader::Extract` implementations. Conventions and deliberate differences:
  - **`GetBulkArchive` returns a small `BulkArchiveRef` rather than an out-param.** Two of its arms hand
    back a *borrowed* archive (the saved one) and three hand back one this port has to own (a sidecar
    payload archive opened on the spot); C# lets the GC paper over that difference, C++ cannot.
  - `GetPayload` likewise returns an **owning** `unique_ptr`, not a borrowed pointer. The payload *map*,
    though, is `shared_ptr`-held so clones share it — matching C#'s explicit "carry over the payloads dict
    to the cloned instance", which is a documented performance decision there.
  - C#'s `Lazy<T[]?>` becomes an explicit `std::optional` cache filled on first `Data()` call. Same
    laziness, and the same three-way distinction between *not read yet*, *read and empty* (a zero-size or
    `BULKDATA_Unused` payload, which C# returns as an empty array) and *read and failed* (null).
  - **`PropertyUtil` replaces .NET reflection with an overload set.** C#'s `FPropertyTagType.GetValue(Type)`
    coerces a stored value to a runtime `Type`; the port has one `PropertyValue(tag, T&)` arm per shape C#
    handles — exact scalar, struct fallback, object index, array, enum. Integer *widening* within one
    signedness is folded in (so `GetOrDefault<uint32_t>` off a `UInt16Property` works as call sites expect)
    while a signedness mismatch still fails, matching C#'s `is T` test. The one real gap: C# resolves an
    `EnumProperty`'s stored text back to a member by reflecting over the enum, and there is no such table
    here, so only the integer-backed form (what cooked unversioned assets actually store) converts. `TODO`.
  - **`WwiseReader` walks a bank as (tag, length, payload) sections** and always re-seeks to each section's
    declared end, whatever the handler did — the same contract `WwisePlugin` has. `WwiseDataSource`'s three
    C# case records become one tagged struct; all three of its `ReadDeferredByteData` arms leave the cursor
    past the range, so the section walk stays in step whether the bytes were deferred or copied.
  - `Package` now reads its `DataResourceMap` (the header resolves through it) and both package types
    attach their `ubulk`/`uptnl` payload providers, which `AbstractFileProvider` builds from the sidecar
    `GameFile`s it already located.
  - `WwiseProvider` is now ported too — see its own bullet below.
- **The Sound + MetaSound export trees are ported — 46 files across `UE4/Assets/Exports/Sound` (22) and
  `UE4/Assets/Exports/MetaSound` (24).** `USoundWave` and its `USoundBase`/`USoundWaveProcedural`/
  `USoundSourceBus` line, `USoundCue` + the 20-odd `USoundNode*` graph classes, `USoundClass`, `UDialogueWave`,
  the two `UMetaSoundSource` classes (the registered one under `MetaSound/`, plus the misspelt `UMetaUSoundSource`
  under `Sound/` that C# keeps as a distinct type) and `UMetaSoundPatch`, and the whole `FMetasoundFrontend*`
  struct family. Everything reads through `PropertyUtil`'s `GetOrDefault`/`GetArray`/`GetStructArray` and the
  bulk-data layer, so only three small engine deps were new: `FStripDataFlags`, `FSubtitleCue`, `FFormatContainer`.
  - **`USoundWave`'s streamed-vs-inlined guess is preserved verbatim:** nothing on disk records which it is, so
    the port takes the version option as the starting guess and, if `SerializePlatformData` throws, rewinds the
    cursor, flips the guess and re-reads — the same try/rewind/retry C# does.
  - **Three C#/C++ shape mismatches are resolved with comments rather than "fixed":** `FSubtitleCue` keeps the
    cue's *resolved string* because this port's `FText` is move-only and the property bag is const; `FFormatContainer`
    keys its map with an explicit `FName` comparator (`FName` has no `operator<`); the `[StructFallback]` vectors are
    built with `emplace_back` (`FStructFallback`/`FPropertyTag` are move-only).
  - **Two faithful quirks are kept as-is:** `FMetasoundFrontendClassMetadata` reads `Type` off the *enum's* name
    (`nameof(EMetasoundFrontendClassType)`, not `"Type"`), so it always comes back as the default; and
    `FDialogueContext::operator==` compares two unresolved indices as *not* equal (null == null is false), matching
    C#'s `!left?.Equals(right) ?? true`.
  - **`PropertyUtil` gained a `SetProperty` arm** (`const UScriptSet*`) so Wwise's `FWwiseSwitchContainerLeafCookedData`
    — which keeps its `GroupValueSet` raw — resolves; it had never been forced to instantiate before this slice.
  - `test_sound_metasound` pins the hierarchy, the `[Flags]` helpers, the empty-bag defaults through every new
    `PropertyUtil` arm, and both quirks above. All 64 concrete types are **now registered** in
    `ObjectTypeRegistry` — see the registry bullet above for the naming rules and what real games exercise.
- **The FMOD bank tree is ported — 105 files under `UE4/FMod` (plus `FModProvider`, below).** `FModReader`
  walks a `.bank` (a RIFF file of form type `"FEV "`) as a flat stream of (4-byte tag, 4-byte size, payload)
  chunks, dispatching each `ERIFFID` to its node reader: the event/timeline/parameter/parameter-layout nodes,
  the bus family (master/group/input/return/output-port), the effect family (built-in/plugin/send/sidechain/
  spectral-sidechain/parameterized), the ten instrument types, transitions, modulators and their six subnodes,
  curves, mappings, properties, controllers, snapshots and VCAs — plus the `FMT`/`BNKI`/`STDT`/`STBL`/`HASH`/
  `SNDH`/`PLAT` metadata chunks and the 29 `Objects/` value structs. `Utils/EventNodesResolver` then walks an
  event's node graph to collect the waveforms it references, and `Utils/JenkinsHash` (FMOD's `hashlittle2`)
  keys the sound table. Covered by `tests/test_fmod_bank.cpp`.
  - **`BinaryReader` is not `FArchive`, and here that is a correctness trap, not a style note.** C# reads this
    format with `System.IO.BinaryReader`, whose `ReadBoolean`/`ReadByte` consume exactly **one** byte —
    `FArchive::ReadBoolean()` consumes **four**. Every bool and byte field in this tree is one byte, so the
    port never calls `ReadBoolean` in `UE4/FMod`: it uses `Ar.Read<uint8_t>() != 0`. Getting one wrong shifts
    everything after it, and the chunk walker's corrective `Position = nextNode` seek at the end of each chunk
    would *hide* the damage rather than report it — which is why the test asserts parsed values and cursor
    positions, not just that parsing completed.
  - **The C# tree is circular** — every node reader consults `FModReader.Version` and its static read helpers,
    while `FModReader` holds dictionaries of those node types. The port breaks it in `FModReader.h`, which
    **forward-declares** all 16 node types and holds them as `unordered_map<FModGuid, unique_ptr<Node>>`, so
    the header the nodes include never includes the nodes. `~FModReader()` is therefore out-of-line in
    `FModReader.cpp` (the one TU that sees the complete types); `unique_ptr` to an incomplete type is legal
    right up to the point the destructor is instantiated.
  - **`FModGuid` is not laid out like `FGuid`.** The conversion rotates `B` by 16 bits and byte-reverses `C`
    and `D` to reach FMOD's on-wire ordering. It is a dictionary key throughout the tree, so it carries a
    `std::hash` specialization.
  - **The three "global readers" carry all the format's variable-length encoding, and two of them differ only
    in where one field sits.** `ReadX16` is an `int16` whose high bit means "a following `uint16` supplies
    bits 15+". `ReadElemListImp` and `ReadVersionedElemListImp` share the same count encoding
    (`ReadX16() >> 1`) but the `uint16` payload size appears **once before the list** vs. **once per element**.
    Each has a getter overload for lists of bare primitives, where C# passed a lambda.
  - **The FSB5 sample decode is out of scope, and `FModProvider` is blocked on it.** C# hands each `SND` chunk's
    (optionally obfuscated) FSB5 stream to the external **Fmod5Sharp** NuGet package, which is not part of
    CUE4Parse's own source. Following the `FDeferredByteData` precedent from the Wwise port, `SoundDataNode`
    keeps the raw FSB5 bytes plus the sub-sound count read out of the header (`FModSoundBank`), and
    `EventNodesResolver` resolves to an `FWaveformRef {SoundBankIndex, SubsoundIndex}` instead of a decoded
    `FmodSample` — bounds-checked against that count, which is exactly what C# checks against
    `bank.Samples.Count`. `Fsb5Decryption` (bit-reverse then XOR with a repeating key) **is** ported: C#
    applies it lazily per read using the absolute stream position, and because the port reads the whole FSB
    region into a buffer starting at position 0 the buffer index equals that position, so one whole-buffer
    transform is equivalent. `FWaveformRef` now lives in its own header (`UE4/FMod/Utils/FWaveformRef.h`)
    because `FModReader` and `EventNodesResolver` both name it and the latter includes the former.
  - **Eager vs. lazy reads are not interchangeable, and one place proves it.** C#'s `SoundDataNode` takes a
    `Substream(fsbOffset, size)` — a *lazy* view that is allowed to name more bytes than the underlying
    stream has, because `FsbLoader` only ever reads the front of the FSB5 and the tail is never touched.
    That is not an edge case: an `SND` chunk's declared size is measured from the chunk **body**, while the
    FSB5 starts `relativeOffset` bytes further in (the `SNDH` header carries the absolute offset), so on a
    real bank the last `SND` chunk *always* overruns the file by exactly that much. The port read eagerly
    and threw `"Read size is bigger than remaining archive length"` on **every shipped bank tested**; the
    read is now clamped to what is actually there. Verified against a retail Subliminal install: all 7 banks
    went from 0 events parsed to 867. Pinned by `TestSoundDataOverhang` in `tests/test_fmod_bank.cpp`.
  - **`FPackedNode.cs` is folded into `FRadixTreePacked.h`** (C# splits the class across two files with
    `partial`); `FModSoundBank.h` is the one file with no C# counterpart, standing in for the Fmod5Sharp types.
  - Faithful quirks kept with comments rather than "fixed": `TransitionRegionNode` reproduces C#'s
    `ReadElemListImp<…>().FirstOrDefault()` as "the first element, or a default-constructed struct";
    `ParseNodes` keeps the shift-back-by-3 recovery for a chunk that starts on a stray null terminator, and
    the `visitedSoundNode` early break for banks that write duplicated FSB data outside an `SND` chunk;
    `FBankInfo::FileVersion` is assigned from `FModReader::Version()` rather than read from the chunk.
- **Both audio providers are ported — `WwiseProvider` and `FModProvider`, the last two files in their
  trees.** These are the layer above the two bank readers: not "parse one container" but "find every
  container a game ships, join them into one index, and answer *which sounds does this export play?*".
  - **Wwise resolves an event two completely different ways, and which one runs depends on how the game was
    cooked.** A modern `UAkAudioEvent` carries an `FWwiseLocalizedEventCookedData` that names its media and
    banks outright, so the answer is read straight off the export. An older one carries only a `ShortID` — or
    nothing at all, in which case the id is the **FNV-1 hash of the export's own name** — and the answer has
    to be *found*, by walking the flat id→node table built from every bank's HIRC section: event → actions →
    switch/random/layer/music containers → decision trees → leaf sources. Both paths are ported, including
    the switch-state stack that `SetSwitch` actions push and pop around an event's action list, and the
    commented-out actor-mixer arm ("skip mixers cause it resolves too many sounds from other events").
  - **That second path is why this works without mappings.** The hierarchy route reads the export's *name*,
    which comes from the package's export map, not from tagged properties — so it resolves even on a game
    whose `.usmap` is unreadable, where every typed field sits at its default. It is the one substantial
    reader in the tree that does not depend on the mappings problem.
  - **Ownership is the real difference from C#.** C# holds GC references to the `WwiseReader`s it parses and
    to the `Hierarchy` objects inside them. Here the provider owns every reader it creates (`_ownedReaders`)
    and the hierarchy tables hold *non-owning* pointers into them; readers that come from an asset library
    belong to the package the provider cached, which outlives it either way. Media stay
    `shared_ptr<FDeferredByteData>`, so a `WwiseExtractedSound` is still just a name plus a promise of bytes —
    nothing is decoded, and the `.wem` is read out of the pak only when someone asks for it.
  - C#'s local functions (`TraverseAndSave`, `SaveWemSound`, `TraverseSwitchContainer`,
    `TraverseDecisionTreeNode`) become private members plus an `EventWalk` struct carrying what those
    closures captured — a C++ lambda cannot recurse without that. `switch (hierarchy.Data)` over C# type
    patterns becomes an ordered `dynamic_cast` chain; the ordering is load-bearing only where two container
    types share a base.
  - **Faithful quirks kept**: `ExtractAudioEventSounds`' last two loops iterate a bank list while ignoring
    the loop variable, so an event with *N* sound banks re-resolves the same event *N* times; `SaveWemSound`
    uses the non-short-circuiting `|` so both lookups always run; `ProcessMediaCookedData` still appends the
    entry when it could not find any bytes for it. One thing genuinely cannot work yet: `RequiredBank` is
    normally an **import**, and this port's `ResolvedImportObject::Object()` still returns null, so that
    branch falls through to the FNV hash — which is exactly what a failed `TryLoad` does in C#.
  - **`FModProvider` merges twice, and both merges matter.** FMOD splits one logical bank across several
    files — metadata in `Foo.bank`, audio in `Foo.assets.bank`, localised audio in `Foo.<lang>.bank` — so it
    groups by the name up to the first `.`, merges each group into one reader, then merges *again* by bank
    GUID so a pak copy and an on-disk copy land together. Both discovery routes are ported: banks inside the
    paks (any path containing `FMOD`) and banks loose on disk under `<game>/Content/FMOD/Desktop`, whose
    location can be overridden by `BankOutputDirectory` in `DefaultEngine.ini`. The `StudioBankKey` from that
    same ini is read through a port of `Regex.Unescape` and feeds `Fsb5Decryption`.
  - Where C# hands back decoded bytes and a file extension from `RebuildAsStandardFileFormat`, a
    `FModExtractedSound` carries the sample's *identity* instead — its `FWaveformRef` plus the
    `FModSoundBank` holding the raw FSB5 container. `ExtractAudioSamples` still filters, just one step
    earlier: where C# drops a sample whose rebuild failed, the port drops a reference whose subsound index
    is past the container's sample count. Names are always the `{fallback}_{i}` form, since the real ones
    live in the FSB5 name table only the decoder reads.
  - C# takes an `IFileProvider`, whose interface declares the `Files` dictionary, the config inis and
    `TrySaveAsset`; this port's `IFileProvider` is far slimmer and those live on `AbstractFileProvider`, so
    that is what `FModProvider` takes — which is what every real caller passes anyway.
  - **What real games say.** Satisfactory (2,497 `.bnk`, 6,258 loose `.wem`): **2,470 of 2,502
    `UAkAudioEvent` exports resolve to at least one `.wem`, 11,370 media references**, and sampled reads pull
    32 KB–231 KB of real bytes back out of the pak. Notably that game's `.usmap` is unreadable, so *no*
    tagged property anywhere resolves — the Wwise hierarchy route still works because the event id falls back
    to the FNV hash of the export's name, which comes from the export map. Subliminal (UE 5.7, FMOD): all 7
    shipped banks parse to 867 events / 2,107 waveform resources / 6 FSB5 containers, and **776 of 786
    `UFMODEvent` exports resolve to at least one sample**. Both are covered hermetically by
    `tests/test_audio_providers.cpp`, which authors a real v145 `.bnk` and a v0x83 `"FEV "` bank into a pak
    and walks the whole path — bulk init, hierarchy index, event traversal, deferred byte read.
- **Not yet ported** (arrive with their layers): the
  dependency-graph `ExportLoader` (non-lazy) loading path, `.locres`
  localization loading (`FTextLocalizationResource`), the full `UStruct`/`UClass`-aware `ConstructObject`
  traversal (a name-keyed `ObjectTypeRegistry` stands in for it), the custom-version providers
  (`FEditorObjectVersion` etc.; property readers currently assume modern), game-specific property readers
  (Borderlands4/OuterWorlds2/…), the named-struct table entries in `FScriptStruct` whose struct types are not ported yet,
  further concrete `UExport` subclasses (`UStringTable`, `UDataTable`, `UCurveTable`, `UScriptStruct`, `UEnum`,
  `UFunction`, `UObjectRedirector`, `UBlueprintGeneratedClass`, `UUserDefinedStruct`, `UUserDefinedEnum`,
  `UCurveFloat`/`UCurveVector`/`UCurveLinearColor` so far; `UStruct`/`UClass`/`UCurveBase` ported but unregistered).
  Marked with `TODO` at their sites.

### Texture tree

- **`TBulkData` spells out all five special members.** Its `virtual ~TBulkData() = default` would otherwise
  suppress the implicit move operations, silently turning every `bulk = TBulkData(Ar)` into a copy. Every
  member is a value type, so the defaults are the right semantics.
- **`Enum.TryParse<EPixelFormat>` becomes `PixelFormatUtils::TryParsePixelFormat`**, backed by a generated
  case-insensitive name table covering all 97 members (numeric strings accepted too, as in C#). The table has
  to stay complete: a missing name makes a cooked texture read `PF_Unknown` and mis-size its mips.
- **`FTexture2DMipMap::BulkData` is a `unique_ptr`**, not a value, because `EnsureValidBulkData` may replace
  an `FByteBulkData` with an `FByteArrayData`. That method currently returns `false` — the landscape path it
  needs (`ULandscapeTextureStorageProviderFactory`) is not ported.
- **`UTexture::LightingGuid` defaults to a plain `FGuid`.** C# uses `new FGuid((uint)GetFullName().GetHashCode())`,
  which is not reproducible here on two counts: `GetFullName` is unported, and .NET Core randomises
  `string.GetHashCode` per process, so the C# value is not even stable across runs.
- **`FEditorBulkData`'s `StoredInPackageTrailer` branch returns an empty payload.** `Package::Trailer` is
  unported, which is the same answer C# gives for a package with no trailer.
- **`CMaterialParams2` is forward-declared only** in `UUnrealMaterial.h`. A pure-virtual reference parameter
  needs no complete type, and both of `UTexture`'s `GetParams` overrides are empty in the C# as well; the
  real 353-line table belongs to the material slice.
- **`FVirtualTextureBuiltData` gained a defaulted default constructor.** C# only has the reading one and
  models "not read" as a null reference; the default-constructed object is exactly that state
  (`!IsInitialized()`).
- **`FVirtualTextureBuiltData::IsLegacyData()` drops half of C#'s test.** C# asks
  `TileOffsetInChunk == null || TileOffsetInChunk.Length > 0`; there is no null vector here, and the only
  path that leaves the field unread is a branch C# has commented out, so an empty vector means the legacy
  arrays *were* read and were empty — the same answer C# gives for a zero-length array.
- **Fully-qualified `CUE4Parse::Utils::…` and `CUE4Parse::UE4::Readers::…` inside the texture namespace.**
  Unqualified, `Utils::` binds to `CUE4Parse::UE4::Assets::Utils` and `Readers::` to `Assets::Readers`.

### Zstd

The Zstd codec is the reference **native libzstd, loaded at runtime** by `ZstdHelper::Initialize` — the same
shape as `OodleHelper`, and unlike Zlib/Gzip/LZ4, which are implemented in-tree. C# reaches for the managed
`ZstdSharp` package here; a hand-written decoder was tried and dropped, because a subtly wrong entropy table
decodes megabytes correctly before drifting, which is a bad trade against a dependency that is already a
runtime `.dll`. Nothing Zstd-shaped decodes until `Initialize` succeeds, and `tests/test_zstd.cpp` reports
SKIPPED rather than failing on a machine without the library.

### Native libraries

Oodle, Zstd and Brotli are runtime `.dll`/`.so` loads, resolved by `CUE4Parse/Compression/NativeLibrary.{h,cpp}` in
this order: an explicit path → `$FMODELCPP_NATIVE_DIR` → the executable's own directory → the in-tree
`ThirdParty/native/<platform>` folder (walking up from the build tree) → the bare name, left to the OS
loader. **No absolute path is compiled in**, and CMake copies whatever sits in `ThirdParty/native/<platform>`
next to the built binaries. C# has no equivalent — .NET's probing rules cover it there, and FModel downloads
Oodle and Zlib-ng at runtime into its own data directory. Brotli has no C# helper file at all to mirror
(`System.IO.Compression.BrotliStream` is in the BCL), so `BrotliHelper` is an addition, not a port. See [ThirdParty/README.md](ThirdParty/README.md).

### Unported files

Every C# file without a port has a placeholder header at its mirrored path (`// Stub for CUE4Parse/…`,
`#pragma once`, the namespace, a `TODO`). They declare nothing, nothing includes them, and the CMake `*.cpp`
glob never sees them. **File existence therefore says nothing about porting status** — check whether the
first line begins `// Stub for`.

## Build

Requires CMake ≥ 3.20 and a C++20 compiler (tested with MSVC 19.38 / VS 2022, Ninja).

```powershell
# from a VS x64 developer prompt (or after running vcvars64.bat)
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

`tests/test_reader.cpp` exercises the reader primitives and the version subsystem.
