# FModelCPP

A slow, faithful C++ port of [FModel](../FModel) and its parsing core **CUE4Parse** (originally C#).

The goal is to stay as structurally close to the C# source as practical: same folder
layout, same namespaces (`CUE4Parse::UE4::Readers`, ...), same type and method names.

The FModel app (originally WPF) is ported on **Qt 6 Widgets** and lives under `FModel/`. It keeps the same
layout and structure as the C# UI. See [The FModel app](#the-fmodel-app) below.

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
  The deep wiring (ApplicationViewModel / CUE4Parse provider / thread worker) is **not** ported yet — menu actions
  and controls are inert placeholders that log to the output pane.
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
  other dependency-free plain enums live in `FModel/Enums.h` (ported from `Enums.cs`; the `[Description]` and
  `AssetCategory` enums are deferred). Behaviour is covered by `FModel/tests/test_framework.cpp`
  (QtTest, registered with ctest).

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
| Version container | `UE4/Versions/VersionContainer.cs` | `UE4/Versions/VersionContainer.h` (core only) |
| Texture platform | `UE4/Assets/.../ETexturePlatform.cs` | `UE4/Assets/Exports/Texture/ETexturePlatform.h` |
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
| Struct value / fallback | `UE4/Assets/Objects/FScriptStruct.cs`, `FStructFallback.cs` | `UE4/Assets/Objects/FScriptStruct.{h,cpp}`, `FStructFallback.{h,cpp}` (fallback path) |
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
| FField / FProperty system | `UE4/Objects/UObject/{FField,UnrealType}.cs` | `UE4/Objects/UObject/{FField,UnrealType}.{h,cpp}` |
| UStruct family | `UE4/Objects/UObject/{UField,UStruct,UScriptStruct,UEnum,UFunction,UClass}.cs` | same paths under `FModelCPP/.../UObject/*.{h,cpp}` |
| UObjectRedirector | `UE4/Assets/Exports/UObjectRedirector.cs` | `UE4/Assets/Exports/UObjectRedirector.{h,cpp}` |
| UBlueprintGeneratedClass | `UE4/Objects/Engine/UBlueprintGeneratedClass.cs` | `UE4/Objects/Engine/UBlueprintGeneratedClass.{h,cpp}` |
| UUserDefinedStruct / UUserDefinedEnum | `UE4/Objects/Engine/UUserDefined{Struct,Enum}.cs` | `UE4/Objects/Engine/UUserDefined{Struct,Enum}.{h,cpp}` |
| Reflection flag enums | `UE4/Objects/UObject/{EStructFlags,EClassFlags,EFunctionFlags}.cs` | `UE4/Objects/UObject/{EStructFlags,EClassFlags,EFunctionFlags}.h` + `CoreNetTypes.h` |
| Provider object loading | `FileProvider/AbstractFileProvider.cs` (`LoadPackageObject`) | `FileProvider/IFileProvider.{h,cpp}` (`LoadPackageObject`/`TryLoadPackageObject<T>`) |

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
- **Decompression is a registry, not hard-wired codecs.** C# builds a decompressor from external
  native packages (Oodle, K4os LZ4, ZstdSharp, Zlib-ng). Those aren't vendored here yet, so
  `Compression` keeps the same public API (`Decompress` overloads, `LOADING_COMPRESSION_CHUNK_SIZE`)
  but routes each algorithm through a registered `DecompressFunc` (mirroring C#'s
  `DecompressorBuilder.Add`). `None` works out of the box (a plain copy); every other algorithm
  throws `UnknownCompressionMethodException` until a codec is registered via `RegisterDecompressor`.
  Wire real codecs in there when their libraries are added — no call-site changes needed.
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
  `/Script/` packages, and misses fall back to the in-package `ResolvedImportObject`; `.locres` loading
  (`FTextLocalizationResource`) and the culture machinery are deferred, so the table is populated via
  `Override`. All marked `TODO`. **`LoadPackageObject`/`TryLoadPackageObject<T>` are now ported** (in
  `IFileProvider.cpp`): they split a path into (package, object) like C#'s `GetPathName`, load the package, and
  return the named export cast to `T*` — used by `StringTableEntry` below.
- **Concrete export types build through an `ObjectTypeRegistry`.** C# discovers
  `UObject` subclasses by reflection; C++ has none, so `ObjectTypeRegistry` (`UE4/Assets/ObjectTypeRegistry.{h,cpp}`)
  keeps a manual name→factory map (each type registers under its serialized class name, e.g. `StringTable`, `DataTable`).
  `Package::ConstructObject` now consults the registry with the resolved class object's name — the same string
  C#'s `UScriptClass.ConstructObject` ultimately looks up — so a `StringTable` export deserializes into a
  `UStringTable` (its `FStringTable`: namespace + key→string entries, plus the metadata map) and a `DataTable`
  into a `UDataTable` instead of a bare `UObject`. Unregistered classes still fall through to base `UObject`.
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
  Double/Name/Str). Deliberate differences: the **unversioned** property path (`FUnversionedHeader` +
  mappings provider) throws for now; the UE5 `PROPERTY_TAG_COMPLETE_TYPE_NAME` tag layout throws; the
  reflection-driven `GetValue<T>`/`PropertyUtil.GetOrDefault<T>` accessors and the JSON `WriteJson` path are
  omitted (consumers read the `Properties` list and `dynamic_cast` a tag's value); `GenericValue` is dropped.
  The tagged-property value set is now complete except for game-specific readers (Borderlands4/OuterWorlds2/…),
  which return null. All `TODO`.
- **The aggregate property types are started** (`StructProperty`, `ArrayProperty`, `EnumProperty` + their value
  types `FScriptStruct`, `FStructFallback`, `UScriptArray`). A struct value reads recursively as a bag of
  tagged properties (`FStructFallback` → `DeserializePropertiesTagged`); an array reads its element count then
  each element via the same `ReadPropertyTagType` factory, including the classic `INNER_ARRAY_TAG_INFO` inline
  tag that carries a struct array's element type. Deliberate differences: `FScriptStruct`'s ~200-entry
  named-struct switch (`FVector`/`FGuid`/`FColor`/…) is **entirely deferred** — every struct takes the C#
  `default:` `FStructFallback` path until the Core/Math + engine struct types are ported, so `StructType` is a
  concrete `FStructFallback` rather than an `IUStruct` base. `UScriptArray`'s `InnerTypeData` paths (unversioned
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
  back to a synthetic `StrProperty "UNK_Entry_i"`). The pre-`PROPERTY_TAG_SET_MAP_SUPPORT` game-specific
  key/value/inner type inference, the `MapStructTypes` table (`InnerTypeData`/`ValueTypeData`), and the many
  game-specific set `InnerTypeData` inferences are deferred (struct keys/values/elements still read via
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
- **Not yet ported** (arrive with their layers): the unversioned-property path + mappings provider, the
  dependency-graph `ExportLoader` (non-lazy) loading path, the concrete `IFileProvider` (`DefaultFileProvider`
  + the VFS/`GameFile` layer; only a minimal in-memory-testable interface exists) with its `.locres`
  localization loading (`FTextLocalizationResource`), the full `UStruct`/`UClass`-aware `ConstructObject`
  traversal (a name-keyed `ObjectTypeRegistry` stands in for it), the custom-version providers
  (`FEditorObjectVersion` etc.; property readers currently assume modern), game-specific property readers
  (Borderlands4/OuterWorlds2/…), the named-struct table in `FScriptStruct` (needs the rest of Core/Math — `FVector`/`FQuat`/… — plus engine structs; `FColor`/`FLinearColor` are done),
  further concrete `UExport` subclasses (`UStringTable`, `UDataTable`, `UCurveTable`, `UScriptStruct`, `UEnum`,
  `UFunction`, `UObjectRedirector`, `UBlueprintGeneratedClass`, `UUserDefinedStruct`, `UUserDefinedEnum`,
  `UCurveFloat`/`UCurveVector`/`UCurveLinearColor` so far; `UStruct`/`UClass`/`UCurveBase` ported but unregistered),
  `IoPackage` (IO Store), and
  `VersionContainer` Options/MapStructTypes tables. Marked with `TODO` at their sites.

## Build

Requires CMake ≥ 3.20 and a C++20 compiler (tested with MSVC 19.38 / VS 2022, Ninja).

```powershell
# from a VS x64 developer prompt (or after running vcvars64.bat)
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

`tests/test_reader.cpp` exercises the reader primitives and the version subsystem.
