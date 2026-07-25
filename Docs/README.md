# FModelCPP — port status & roadmap

*Updated 2026-07-25*

A slow, structure-faithful port of **FModel** (the Unreal Engine asset explorer) and its parsing core
**CUE4Parse**, from C#/WPF to C++/Qt. Two tracks advance in parallel: the headless parsing library, and the
desktop app built on top of it.

See the root [README](../README.md) for the per-file porting table and the catalogue of deliberate
differences from the C# source.

## Progress

| Track | Progress | Size | Notes |
| --- | --- | --- | --- |
| **CUE4Parse** — parsing core | `██████████████░░░░░░` **~72%** | 909 real C++ files (+1,041 stubs) · 51 test suites | Measured against the *reading path* |
| **FModel** — desktop app | `██▌░░░░░░░░░░░░░░░░░` **~13%** | 45 C++ files · 3 test suites · Qt 6.8 Widgets | |

**CUE4Parse.** The bottom-up reading spine is complete through the tagged-property system, and both package
formats (classic `.uasset` and Zen/IO Store) now load exports end to end. Both soundbank formats are in as
well — the Wwise tree (archive, bank objects, ~50 plugin parameter sets, the whole HIRC hierarchy) and the
FMOD `.bank` chunk walker with its node graph.

Three different denominators, all true: **~72%** of the reading path, the running per-layer tally sits nearer
**~78%**, and by strict same-relative-path file count it is **758 of the 1,799 C# files (42%)**. The strict
count runs below the tally because a handful of C# files fold into another header rather than getting their
own (`FPackedNode.cs`, the relocated Wwise leaf types, `PropertyUtil` split out of `UObject.cs`).

The remaining 1,041 C# files now all have a **placeholder header** at their exact mirrored path, carrying
nothing but `#pragma once`, the correct namespace and a `TODO: port …` line. They are not included by
anything and compile into nothing — they exist so the tree shape matches the C# source and so the gap is
visible by browsing rather than by diffing two file lists. A file with real content is ported; a file whose
first line reads `// Stub for CUE4Parse/…` is not.

**FModel.** Window shell, theme, resources, the MVVM foundation, the whole persisted-settings tree and the
root `ApplicationViewModel` are in — the app can now read and rewrite a real `AppSettings.json`. The view /
view-model / rendering layers (228 C# files) are still ahead.

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
  (`LoadIniConfigs`, `GameDisplayName`, `PostMount`) over a vendored `UE4Config` parser. Localization beyond
  the lookup table still deferred
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
AdonisUI Dark theme). A running shell plus the full persisted-settings tree; the controls are not yet bound
to it.

## Foundation — done (8 pieces)

- ✅ **MainWindow shell** — menu bar, 3 left tabs, explorer / editor / log area, status bar
- ✅ **AdonisUI Dark theme** — `Theme.cpp`: Fusion palette + stylesheet, accent `#206BD4`
- ✅ **Resources** — 107 files embedded via `.qrc`
- ✅ **MVVM base** — `ViewModel`, `Command`, `ViewModelCommand<T>` (QObject + QPointer)
- ✅ **FStatus view-model** — wired to the status bar via `propertyChanged`
- 🟡 **Enums** — plain + `[Description]` enums ported (as a `description()` overload set); asset-category
  enum deferred
- ✅ **Settings tree** — `UserSettings` / `DirectorySettings` / `EndpointSettings` / `VersioningSettings` /
  `CustomDirectory`; reads & rewrites a real `AppSettings.json` as a fixed point
- ✅ **WPF value types** — `Hotkey` + `Key`/`ModifierKeys` (WPF's numeric values, via `Q_ENUM_NS`) and
  `GridLength` — needed for on-disk format compatibility

## Remaining — phased (~215 files)

### Phase 1 · View-models

*The logic layer behind every control.*

- ⬜ **ViewModels** *(45)* — `ApplicationViewModel` + `LoadingModesViewModel` done; `CUE4ParseViewModel`,
  `ThreadWorker`, tab/search and all commands still ahead
- 🟡 **API endpoints** *(13)* — the response DTOs `UserSettings` stores are in; the endpoints and HTTP layer
  are not

### Phase 2 · Dialogs & core views

- ⬜ **Settings, Directory selector, AES manager**
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
| 1 | core | **`LoadVirtualPaths` + plugin manifests** | The last unported piece of `AbstractFileProvider`: `.upluginmanifest`/`.uplugin` JSON, which is what makes `FixPath`'s virtual-root branch do anything for plugin content. |
| 2 | app | **The command layer** | `MenuCommand` / `CopyCommand` / `RightClickMenuCommand` on top of the ported `ViewModelCommand<T>` base — replaces `MainWindow`'s `onMenuCommand` stand-in, and is what the now-live `ApplicationViewModel` was missing. |
| 3 | app | **Settings dialog** | `SettingsViewModel` + the view, binding the settings tree that just landed — the first place the ported `description()` overloads earn their keep. |

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

`U:\Programs\AI\Claude\FModelCPP` · MSVC + Ninja · Qt 6.8.3 · **51 test suites green** · faithful C#→C++, bottom-up

> **Declaring the engine version is not optional.** A wrong `EGame` does not fail cleanly — the Zen summary,
> the bulk-data map and the property path are all version-gated, so an off-by-one version reads plausible
> bytes at the wrong offsets and dies somewhere unrelated. When a real-game run fails, check the version
> before suspecting the reader: `Engine/Build/Build.version`, or the shipping binaries' `ProductVersion`.
