# FModelCPP — port status & roadmap

*Updated 2026-07-26*

A slow, structure-faithful port of **FModel** (the Unreal Engine asset explorer) and its parsing core
**CUE4Parse**, from C#/WPF to C++/Qt. Two tracks advance in parallel: the headless parsing library, and the
desktop app built on top of it.

See the root [README](../README.md) for the per-file porting table and the catalogue of deliberate
differences from the C# source.

## Progress

| Track | Progress | Size | Notes |
| --- | --- | --- | --- |
| **CUE4Parse** — parsing core | `██████████████░░░░░░` **~72%** | 914 real C++ files (+1,035 stubs) · 53 test suites | Measured against the *reading path* |
| **FModel** — desktop app | `██████░░░░░░░░░░░░░░` **~28%** | 83 C++ files · 7 test suites · Qt 6.8 Widgets | **Loads a real game end to end** |

**CUE4Parse.** The bottom-up reading spine is complete through the tagged-property system, and both package
formats (classic `.uasset` and Zen/IO Store) now load exports end to end. Both soundbank formats are in as
well — the Wwise tree (archive, bank objects, ~50 plugin parameter sets, the whole HIRC hierarchy) and the
FMOD `.bank` chunk walker with its node graph.

Three different denominators, all true: **~72%** of the reading path, the running per-layer tally sits nearer
**~78%**, and by strict same-relative-path file count it is **764 of the 1,799 C# files (42%)**. The strict
count runs below the tally because a handful of C# files fold into another header rather than getting their
own (`FPackedNode.cs`, the relocated Wwise leaf types, `PropertyUtil` split out of `UObject.cs`).

The remaining 1,035 C# files now all have a **placeholder header** at their exact mirrored path, carrying
nothing but `#pragma once`, the correct namespace and a `TODO: port …` line. They are not included by
anything and compile into nothing — they exist so the tree shape matches the C# source and so the gap is
visible by browsing rather than by diffing two file lists. A file with real content is ported; a file whose
first line reads `// Stub for CUE4Parse/…` is not.

**FModel.** Window shell, theme, resources, the MVVM foundation, the whole persisted-settings tree, the root
`ApplicationViewModel` and its command layer are in — the app can now read and rewrite a real
`AppSettings.json`, and every menu entry routes through the real `MenuCommand` instead of a stand-in. The
**Settings dialog** is the first window on top of that: `SettingsViewModel` + `SettingsView` + the
`DictionaryEditor`/`EndpointEditor` it hosts, so *Settings → Settings* opens the real four-tab dialog, edits
the live settings tree and saves it — the first `MenuCommand` arm to stop being deferred. The **explorer's
folder tree** followed: `AssetsFolderViewModel` / `TreeItem` / `AssetsListViewModel` turn a flat provider
file list into the nested, sorted, searchable structure the left pane binds to, and the command layer's
folder half — the TreeItem arm, the collapse walk, the folder/asset selection split — is no longer deferred.

**The app now loads a game.** `CUE4ParseViewModel` (the provider host), `ThreadWorkerViewModel` (the job
funnel), `GameDirectoryViewModel` (the Archives tab), `AesManagerViewModel`, `LoadCommand` and the two
windows the flow needs — `DirectorySelector` and `AesManager` — are in, wired through `ApplicationViewModel`
and driven by `MainWindow` on startup. Pick a directory, supply its AES keys, press Load, and the folder
tree fills. By the same strict file count as above, 44 of the app's 233 C# files are ported and **189 still
carry a placeholder header**: the asset viewers, the export path and the whole rendering layer.

### Verified on a real game

The container → package path is exercised end to end against a retail **Satisfactory 1.2.0 (UE 5.6)**
install — 2 IO Store containers, 48,616 files. With an Oodle library supplied to `OodleHelper::Initialize`,
**all 23,699 `.uasset` files extract and parse** (chunk resolution, Oodle + uncompressed block loops, Zen
summary, name batch, import/export maps), and 110,664 export objects construct. `.usmap` parsing is
cross-checked against 18 real mappings files spanning UE 4.20 → 5.7 and usmap versions 0/1/3/4: the port
parses 15 and rejects 3, matching an independent reference parser on every file — including *which* three
are unreadable and at which byte. Those three are written by dumpers whose format CUE4Parse does not
implement (one carries 784 KB of trailing data), so the port deliberately fails them exactly as the C# does
rather than guessing.

With a native libzstd loaded, the mappings path was re-run over a fresh set of 21 real `.usmap` files
(`G:\TempBEProcess`, plus dumper output and a Zstd-compressed Fortnite mapping): **all 21 parse**, including
usmap compression methods None and ZStandard — the Fortnite `_zs` file alone yields 38,066 types and 5,589
enums. With all three native codec libraries present — Zstd, Oodle and Brotli, dropped into
`ThirdParty/native/win-x64` — **31 of 32 real `.usmap` files parse**, covering every usmap compression
method (None, Oodle, Brotli, ZStandard) and up to 42,498 types each. The single failure is a zero-byte file.

The localization path is verified on the same Satisfactory install, and it needs no mappings — `.locres` is
a standalone format, not a package. The provider finds 224 `.locres` and 9 `.locmeta` files; the Engine
`.locmeta` reports 14 compiled cultures; `LoadLocalization("fr")` sweeps the three matching game targets
(`AllStringTables` 6,423 entries, `Narrative` 2,154, `InputKeys` 348) into **8,925 entries across 55
namespaces**, and the `(?!Engine)` exclusion keeps the 16 Engine targets out. Loading `de` over the same
keys changes the text of **8,406 of the 8,925** — the remaining 519 are strings Coffee Stain ships
untranslated — so the culture really is selecting the file, not just the count.

A second retail install — **Poppy Playtime Chapter 5 (UE 5.6)**, 6 containers, 27,669 files — covers what
Satisfactory cannot: it has a `.usmap` the port *can* read, so unversioned properties actually resolve.
There, 58,993 of 94,685 exports read tagged properties, and the typed audio path runs on real cooked data:
**all 3,000 `USoundWave` exports read a valid `CompressedDataGuid` and a `FStreamedAudioPlatformData` with
non-empty chunks** (`BINKA`, a 28-byte header chunk plus the payload), while the 566 `UMetaSoundSource`
exports correctly carry *no* wave payload — that is `SoundBaseDeserialize` opting out, exactly as in the C#.

> Mappings are load-bearing, not optional, for UE5. Satisfactory's own shipped `.usmap` is one of the three
> unreadable ones, so across its 110,664 exports the port reads **zero** tagged properties — every typed
> `Deserialize` bails at the unversioned header. Container access and property access are separate problems,
> and only the first one is solved for that game.

**Both audio providers are verified on shipped games too**, and they are the one substantial reader that
does *not* depend on the mappings problem. On the same Satisfactory install — 2,497 `.bnk` and 6,258 loose
`.wem` — `WwiseProvider` indexes every bank and **2,470 of the 2,502 `UAkAudioEvent` exports resolve to at
least one `.wem`, 11,370 media references in all**, with sampled reads pulling the real bytes back out of
the pak (32 KB–231 KB each). That works *despite* the unreadable `.usmap`, because with no `ShortID`
property to read the event id falls back to the FNV-1 hash of the export's own name, which comes from the
export map. For FMOD, a retail **Subliminal (UE 5.7)** install: all 7 shipped banks parse (867 events, 2,107
waveform resources, 6 FSB5 containers), and **776 of its 786 `UFMODEvent` exports resolve to at least one
sample** through the `AssetGuid` → bank-GUID join.

> Real banks found a real bug, one layer down. Every one of Subliminal's banks died with *"Read size is
> bigger than remaining archive length"* inside its `SND` chunk. The cause: a chunk's declared size is
> measured from the chunk body, but the FSB5 container starts `relativeOffset` bytes further in, so the size
> always names that many more bytes than the file has. C# never notices because it takes a *lazy*
> `Substream` and its loader only reads the front; the port read eagerly and threw. Clamping the read to
> what is actually there took all 7 banks from 0 events to 867. `tests/test_fmod_bank.cpp` now pins it.

**Legend.** ✅ ported & tested · 🟡 stub or subset in place · ⬜ not started.
Counts in parentheses are **C# source files remaining**.

---

# Track 1 — CUE4Parse, the parsing core

Ported bottom-up so every layer rests on tested foundations. The spine below reads an uncompressed,
unencrypted UE package end-to-end; what remains is real-container access and the asset-type long tail.

## The spine — done (13 layers)

- ✅ **Readers layer** — `FArchive`, byte / pointer / stream / big-endian / random-access + compressed proxy
- ✅ **Versions** — `EGame`, `ObjectVersion`, `FPackageFileVersion`, `VersionContainer` (incl. the per-game
  Options / MapStructTypes tables), `VersionUtils` + the whole `F*{Object,Custom}Version` family
- ✅ **Compression codecs** — Zlib/Gzip/LZ4 built-in (from-scratch DEFLATE + LZ4 block); Oodle/Zstd/Brotli
  via runtime native-library load, resolved with no absolute path compiled in. Zstd is verified against reference-produced frames and a real
  Zstd-compressed Fortnite `.usmap` (38,066 types); see the note under *Deliberate differences*
- ✅ **Core/Misc value structs** — `FGuid`, `FDateTime`, `FEngineVersion`, `FSHAHash` …
- ✅ **Core/Math — colors, vectors, geometry, bounds** — `FColor`, `FLinearColor`, all vector types,
  `FQuat`/`FRotator`/`FMatrix`/`FTransform`/`FPlane`, and `FBox`/`FBoxSphereBounds`/`FSphere`/`TRange` —
  every C# file bar `UnrealMathSSE`
- ✅ **FName name pool** — `FNameEntrySerialized`, `FMappedName`
- ✅ **Package summary + custom versions**
- ✅ **Import / export map** — `ObjectResource`, `FAssetArchive`, `ResolvedObject`
- ✅ **Tagged-property system** — ~30 property types, `FPropertyTag`, `UScript`Array/Map/Set, `FStructFallback`
- ✅ **Package + UObject + first assets** — `UDataTable`, `UCurveTable`, `UCurveLinearColor`, `UStringTable`,
  redirector
- ✅ **Every enum-only file in the tree** — 113 headers: the Wwise (36 + 15 flags) and FMod (16) enum
  directories whole, plus the 46 remaining enum-only files anywhere else — `Kismet`, `RigVM`, `RHI`, `i18N`,
  `Texture`, `Material`, `Animation`, `WorldPartition`, `VirtualFileCache` … Includes the `[Description]`
  tables FModel uses as i18n keys and the hand-written Wwise version-mapping helpers
- ✅ **Wwise soundbank tree** — 158 files: `FWwiseArchive` (version-aware decorator, stz strings, big-endian
  7-bit ints), the FNV-1 hash, `AkBankHeader`/`FAKPKHeader`, ~57 bank object structs incl. the decision tree
  and positioning params, the 15 `CAkAction*` classes, ~50 plugin parameter sets across 9 vendor namespaces
  behind a 75-arm dispatch, and the whole HIRC hierarchy (25 container types) with C#'s rewind-and-reparse
  recovery. **Versions 65–174**
- ✅ **FMOD bank tree** — 105 files: `FModReader`'s `"FEV "` RIFF chunk walker with its parent-stack body
  stitching, the event / timeline / parameter / parameter-layout nodes, the bus (7), effect (8), instrument
  (10), transition (3) and modulator-subnode (6) families, the `Objects/` value structs (29), the metadata
  chunks incl. the packed radix tree that maps GUIDs to `event:/…` paths, FMOD's `hashlittle2`, the FSB5
  bit-reverse/XOR deobfuscation, and `EventNodesResolver`'s graph walk from an event to its waveforms.
  **Bank versions 0x2c–0x92**
- ✅ **Both audio providers** — `WwiseProvider` and `FModProvider`, the layer above the two bank readers.
  Wwise: bulk-loads every `.bnk`/`.pck`/`.wem` a game ships (in the paks and loose beside them), indexes
  every bank's HIRC section into one flat id→node table, and resolves an event either from its cooked-data
  struct or by walking that table from an id — which, when the export carries no `ShortID` at all, is the
  **FNV-1 hash of its own name**, so this path works even on a game with no usable `.usmap`. FMOD: finds
  banks by both routes, merges them twice (by file-name group, then by bank GUID), reads `StudioBankKey` /
  `BankOutputDirectory` out of `DefaultEngine.ini`, and resolves each event's node graph to its waveforms.
  Neither decodes audio — see the note under Gaps
- ✅ **Bulk data & payloads** — `TBulkData`/`FByteBulkData`/`FByteBulkDataHeader` with all three header
  readings (Io bulk-data map, classic data-resource map, inline), `FAssetArchive`'s payload registry and the
  `.ubulk`/`.uptnl`/`.m.ubulk` lookups, the partial-read `header` threaded through `GameFile`/`Extract`, and
  `PropertyUtil` — the `GetOrDefault<T>` seam every cooked asset type reads its fields through

## Remaining — phased (~1,200 files)

### Phase A · Real data access

*Unblocks opening an actual shipped game.*

- ✅ **Encryption / AES** — `Aes` + `FAesKey`; AES-256 ECB written from scratch (C# gets it from the BCL),
  pinned to the FIPS-197 and NIST SP 800-38A vectors
- ✅ **Pak container** — `PakFileReader`, `FPakEntry`, `FPakInfo`; legacy, updated, flat and frozen indices;
  compressed + encrypted extraction. Per-game `GameTypes` hooks throw rather than guess
- ✅ **IO Store** — `.utoc`/`.ucas`: `IoStoreReader`, toc resource, chunk resolution (incl. perfect hash),
  partitioned ucas, directory index, container header, `IoGlobalData` and the `IoPackage` Zen asset layer
- ✅ **Virtual file system** — `GameFile`, `VfsEntry`, `IVfsReader`/`IAesVfsReader`, both abstract readers
  and `MountTo`
- ✅ **FileProvider (concrete)** — `DefaultFileProvider`: directory scan, register/mount/submit-key
  lifecycle, `FileProviderDictionary` read-order shadowing, `FixPath`, and the config-ini surface
  (`LoadIniConfigs`, `GameDisplayName`, `PostMount`) over a vendored `UE4Config` parser
- ✅ **Localization** — `.locres` (all three format versions) and `.locmeta`, the `InternationalizationDictionary`
  culture machinery (`InitFromMeta`, `TryGetCulture`, `ChangeCulture`'s `.locres` sweep) and the provider's
  `LoadLocalization` / `TryChangeCulture` / per-game `GetLanguageCode` table
- ✅ **Mappings provider** — `.usmap`: `UsmapParser`, `TypeMappings`, the file/memory providers, and the
  `FUnversionedHeader`/`FIterator` unversioned property path they feed

### Phase B · Math & memory image

*Geometry types every mesh/material asset needs.*

- ✅ **Core/Math remainder** — bounds + ranges + half vectors landed; only `UnrealMathSSE` is left, and it is
  an SIMD fast path with no unique semantics
- ✅ **FMemoryImageArchive** — frozen/cooked structure reader: arrays, strings, bit arrays, sparse arrays,
  sets and maps. The material-preshader readers wait on the material layer

### Phase C · Asset-type long tail

*The bulk of the codebase — port on demand, by importance.*

- ✅ **Texture** — `UTexture2D` + platform data (the whole 29-file `Exports/Texture` tree, `EPixelFormat`
  + the geometry table, `FTexturePlatformData`, mip chains, virtual-texture built data, 27 registered
  types). Pixel *decoding* (BCn/ASTC/ETC) lives in the separate `CUE4Parse-Conversion` assembly and is
  still ahead.
- ⬜ **Static & skeletal mesh**
- ⬜ **Material / instances**
- ⬜ **Animation + ACL compression**
- ✅ **Sound + Wwise + FMOD** *(complete)* — both bank formats, both export trees and both providers:
  `UE4/Wwise` (212/212), `UE4/FMod` (106/106), and every `Assets/Exports/` `Wwise` (46) / `FMod` (7) /
  `Sound` (22) / `MetaSound` (24) asset type — all 64 concrete types registered in `ObjectTypeRegistry`, so
  a cooked package constructs them instead of a bare `UObject`. `WwiseProvider` and `FModProvider` join the
  bank readers to the export types and answer "which sounds does this event play?"; audio is still never
  *decoded* (a `.wem` comes back as deferred bytes, an FMOD sample as a reference into its FSB5 container)
- ⬜ **Niagara, Landscape, World/Level, …**

### Phase D · Supporting systems

- ⬜ **Asset registry** *(22)*
- ⬜ **GameTypes (per-game overrides)**

---

# Track 2 — FModel, the desktop app

A Qt 6 Widgets port of the WPF app, kept visually and structurally faithful (same `x:Name`s, same layout,
AdonisUI Dark theme). A running shell plus the full persisted-settings tree, the first real window over it
(the Settings dialog), and the view-model behind the explorer's folder tree; the explorer *controls* are not
yet bound to it.

## Foundation — done (11 pieces)

- ✅ **MainWindow shell** — menu bar, 3 left tabs, explorer / editor / log area, status bar
- ✅ **AdonisUI Dark theme** — `Theme.cpp`: Fusion palette + stylesheet, accent `#206BD4`
- ✅ **Resources** — 107 files embedded via `.qrc`
- ✅ **MVVM base** — `ViewModel`, `Command`, `ViewModelCommand<T>` (QObject + QPointer)
- ✅ **FStatus view-model** — wired to the status bar via `propertyChanged`
- ✅ **Enums** — plain + `[Description]` enums ported (as a `description()` overload set), including
  `EAssetCategory` and the `AssetCategoryExtensions` helpers that decode its packed base category
- ✅ **Settings tree** — `UserSettings` / `DirectorySettings` / `EndpointSettings` / `VersioningSettings` /
  `CustomDirectory`; reads & rewrites a real `AppSettings.json` as a fixed point
- ✅ **WPF value types** — `Hotkey` + `Key`/`ModifierKeys` (WPF's numeric values, via `Q_ENUM_NS`) and
  `GridLength` — needed for on-disk format compatibility
- 🟡 **Command layer** — `MenuCommand`, `CopyCommand` and `RightClickMenuCommand` on the `ViewModelCommand<T>`
  base, exposed as the three lazy `ApplicationViewModel` properties WPF binds to. Every arm of all three
  switches is ported; the ones whose target view-model or window is not ported yet raise a `deferred` signal
  instead of acting, which is what `MainWindow` now logs
- 🟡 **Settings dialog** — `SettingsViewModel` + `SettingsView` + the `DictionaryEditor`/`EndpointEditor` it
  hosts: four tabs over the live settings tree, snapshot/diff on save (which reload does the app owe? does it
  need a restart?), and `MenuCommand`'s `Settings` arm now opens it for real. The reload steps it schedules
  wait on `CUE4ParseViewModel`, and the endpoints' *Send*/*Test* wait on the HTTP layer
- 🟡 **Explorer folder tree** — `AssetsFolderViewModel` + `TreeItem` + `AssetsListViewModel` + the row
  view-model `GameFileViewModel`, over two new Framework pieces (`RangeObservableCollection`, and a
  `CollectionView` standing in for WPF's `ICollectionView`). `BulkPopulate` builds the whole nested tree
  from a flat provider file list; search text and the category tabs filter it. `GameFileViewModel` resolves
  a row by *extension* in full — the package-driven type switch and both image previews wait on
  `CUE4ParseViewModel`, the concrete `UExport` tree and a Skia decoder
- 🟡 **The game-loading pipeline** — `CUE4ParseViewModel` (the *loading half* of a 1,800-line file: provider
  selection, Initialize, LoadVfs, ClearProvider, the local-file mappings branch, LoadVirtualPaths,
  LoadLocalizedResources, VerifyConsoleVariables), `ThreadWorkerViewModel` + `AsyncQueue`,
  `GameDirectoryViewModel`, `AesManagerViewModel` + `FullyObservableCollection`, `Helper::fixKey`,
  `LoadCommand`, `GameSelectorViewModel`, and the `DirectorySelector` / `AesManager` windows.
  **Verified against three retail installs — see below.** Not ported: the streamed (live-service) providers,
  the API-backed steps (AES refresh, mappings download, news), the launcher auto-detection, and the two
  backup-driven loading modes

## Remaining — phased (~189 files)

### Phase 1 · View-models

*The logic layer behind every control.*

- ⬜ **ViewModels** *(33)* — `ApplicationViewModel`, `LoadingModesViewModel`, `SettingsViewModel`, the
  explorer tree trio and the three commands above are done; `CUE4ParseViewModel`, `ThreadWorker`, tab/search
  and the nine remaining commands still ahead
- 🟡 **API endpoints** *(13)* — the response DTOs `UserSettings` stores are in; the endpoints and HTTP layer
  are not

### Phase 2 · Dialogs & core views

- ✅ **Settings** — see the foundation list above
- ✅ **Directory selector, AES manager** — the two windows the loading flow needs
- ⬜ **About, Search, Update, Backup manager**

### Phase 3 · Explorer & editor

- ⬜ **Tiled explorer, breadcrumb, context menus**
- ⬜ **Property viewer + code viewer** — AvalonEdit → a Qt syntax editor

### Phase 4 · Creators, viewport & plumbing

- ⬜ **Creators (Skia icon/image gen)** *(30)*
- ⬜ **3D mesh / texture viewport + audio player** — ImGui/OpenGL → Qt OpenGL or a viewport widget
- ⬜ **Services, extensions, logging, API/network, updater** *(~24)*

---

## Recommended next slices

Each is a self-contained, testable unit — in the standing bottom-up order. One per "continue".

| # | Track | Slice | Why |
| --- | --- | --- | --- |
| 1 | core | **The `.usmap` parse failure found by the loading slice** | `Satisfactory/CommunityResources/FactoryGame.usmap` (v1, package-versioned, uncompressed) desyncs ~1.27 MB into the body after 57,909 names read cleanly. A real defect in `UsmapParser`, found by loading a real game, and it blocks unversioned-property reads for that install. |
| 2 | app | **`TabControlViewModel` + the JSON viewer** | With a tree on screen, the next thing a user does is click an asset. This is the first half of that: a tab strip and a text/JSON view over `UObject`'s serialized properties. |
| 3 | core | **`UStringTable` + `FText` culture round-trip** | With the localization table now filled from the game rather than by hand, `FTextHistory::Base` resolves against real data — worth a retail pass over text properties before moving on. |

### Retired: "The game-loading pipeline"

Done, and **verified against three retail installs** by driving the ported view-models headlessly (no key
material is printed or stored by the harness):

| Install | Result |
| --- | --- |
| **Satisfactory** (UE 5.6) | 3 archives registered → **2 mounted**, 48,616 files, project `FactoryGame`, display name `Satisfactory`; the tree built **4,560 nodes holding 41,825 files** under 2 roots; 88 virtual paths |
| **Poppy Playtime Ch5** (UE 5.6) | 7 archives → **6 mounted**, 27,669 files, mappings loaded from `5.6.0-0+UE5-ch5_pro.usmap`, **2,346 localized resources**, 2,854 nodes holding 19,749 files |
| **Dead By Daylight** (UE 5.6) | 339 archives registered, **0 mounted with no working key** — the correct outcome, reported as "an encrypted archive has been found…" rather than an empty tree |

Satisfactory also surfaced a genuine `UsmapParser` defect (slice 1 above): its `CommunityResources` mappings
file throws part-way through the body. The startup path now guards that step — a broken `.usmap` must not
stop a game that has already mounted — which is a deliberate deviation, since C# lets the failure escape
into an `async void`.

### Retired: "`TreeItem` + `AssetsFolderViewModel`"

Done. `TreeItem`, `AssetsListViewModel`, `AssetsFolderViewModel` and the row view-model `GameFileViewModel`
are ported, over two new Framework pieces — `RangeObservableCollection` (the real port) and `CollectionView`
(new port-support code standing in for WPF's `ICollectionView`, like CUE4Parse's `Utils/Json.h` stands in for
Newtonsoft). `BulkPopulate` builds the nested tree, buckets rootless files under a synthetic `Content` node,
selects the project root and un-mutes every collection depth-first. The command layer's folder half is no
longer deferred: `MenuCommand`'s `TreeItem` arm re-selects the folder, `setFoldersIsExpanded` walks the whole
tree, and `RightClickMenuCommand::splitSelection` accepts all three of C#'s selection element types.
`test_assets_folder` (22 slots) covers it. Still deferred: `GameFileViewModel`'s package-driven type switch
and both image previews, and the route from a command to the tree (`CUE4Parse.AssetsFolder`).

### Retired: "The Settings dialog"

Done. `SettingsViewModel`, `SettingsView` and the two editors it hosts (`DictionaryEditor`, `EndpointEditor`)
are ported, plus the `EnumExtensions.GetDescription` fallback and `EGameValues()` (C#'s
`Enum.GetValues<EGame>()`) that the UE-version combo needs. *Settings → Settings* now opens the real dialog
through a new `MenuCommand::setOpenWindowHandler` seam, edits the live settings tree and saves it.
`test_settings_view` (19 slots) pins the snapshot/diff logic, the game-list dedupe + reorder, the JSON
round-trip through `VersioningSettings`, and the two upstream slips the port keeps on purpose (the
compression-format assignment in `Initialize`, and the sticky mappings flag). Still deferred: the four
`CUE4ParseViewModel` reload steps and the restart warning `OnClick` schedules, the endpoints' *Send*/*Test*
(HTTP), and the Creator entry (its view-model is unported, so the row stays hidden).

### Retired: "`.locres` localization"

Done. `FTextLocalizationResource` (Legacy / Compact / Optimized_CRC32, including the string-table
ref-count stealing and the HonorofKingsWorld layout), `FTextLocalizationMetaDataResource`, and the three
i18N value types are ported. `InternationalizationDictionary` gained `InitFromMeta`, `TryGetCulture` and
`ChangeCulture`'s `.locres` sweep; `AbstractFileProvider` gained `LoadLocalization`, `TryChangeCulture` and
the per-game `GetLanguageCode` table. `test_localization` drives every format version off hand-built
archives, and the whole path is verified on retail Satisfactory (see above). Still deferred, each with a
named TODO at the site: the CodeVein2 / EmbersofTheUncrowned / NevernessToEverness encrypted string tables
and the Aion2 `L10NString.dat` branch, all four blocked on classes that are still stubs.

### Retired: "The command layer"

Done. `MenuCommand` / `CopyCommand` / `RightClickMenuCommand` are ported onto `ViewModelCommand<T>` and
exposed as the three lazy `ApplicationViewModel` properties. `MainWindow`'s `onMenuCommand` stand-in is gone:
the menu now runs the real command, four arms act (the three external links and *Open Output Directory*) and
the rest report which port they are waiting on. `RightClickMenuCommand`'s dispatch — the 11-trigger table,
the bulk-type → directory table, the group-by-directory pass with its `Auto` flag and the export-path
builder — is in and tested; the walk it feeds waits on `CUE4ParseViewModel`. `test_commands` covers all
three.

### Retired: "`LoadVirtualPaths` + plugin manifests"

Done. `.upluginmanifest`/`.uplugin` JSON is read through a small in-tree JSON reader
(`Utils/Json.h`, standing in for Newtonsoft), `UE4/Plugins/UPluginManifest.h` is ported, and
`FixPath`'s virtual-root branch now resolves plugin content. `test_virtual_paths` covers both arms,
the `CanContainContent` filter, manifest-wins-over-`.uplugin` precedence and the JSON reader itself.

### Retired: "IO Store uncompressed-block reads"

An earlier run of the stack over the same Satisfactory install reported 462 packages dying with *"Read size
is bigger than remaining archive length"* on chunks stored uncompressed, and this list carried it as the
next slice. **It was not a bug.** The harness had declared the game as UE 5.3/5.4 while the install had
since updated to 1.2.0 / UE 5.6, so `Ar.Ver` fell below `VERSE_CELLS` and `IoPackage` skipped the 8-byte
`FZenPackageCellOffsets` block that those packages do carry. Everything after it read 8 bytes early, the
name batch came out of the cell offsets, and the resulting absurd name count overran the buffer — far from
the actual cause, and in a place that looks like a container or decompression fault. Declaring the right
version parses all 23,699. The gate is now pinned both ways by `tests/test_zen_package.cpp`.

---

`U:\Programs\AI\Claude\FModelCPP` · MSVC + Ninja · Qt 6.8.3 · **57 test suites green** · faithful C#→C++, bottom-up

> **Declaring the engine version is not optional.** A wrong `EGame` does not fail cleanly — the Zen summary,
> the bulk-data map and the property path are all version-gated, so an off-by-one version reads plausible
> bytes at the wrong offsets and dies somewhere unrelated. When a real-game run fails, check the version
> before suspecting the reader: `Engine/Build/Build.version`, or the shipping binaries' `ProductVersion`.
