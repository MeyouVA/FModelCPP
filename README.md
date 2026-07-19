# FModelCPP

A slow, faithful C++ port of [FModel](../FModel) and its parsing core **CUE4Parse** (originally C#).

The goal is to stay as structurally close to the C# source as practical: same folder
layout, same namespaces (`CUE4Parse::UE4::Readers`, ...), same type and method names. The
FModel UI (WPF) is a later phase; it will keep the exact same look.

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
  `UObject` — this port implements `IPackage` directly (no `UObject` yet); object **loading** is deferred
  (no `ExportsLazy`/`ConstructObject`/`DeserializeObject`, no `uexp`/`ubulk` payloads), so `ResolvedObject`
  answers names but not `.Object`/`Load<T>`; the optional summary tables (thumbnails, depends, preload deps,
  soft-object paths, data resources, trailer) are not read yet; cross-package import resolution
  (`Provider.TryLoadPackage`) is deferred, so `ResolveImport` returns the in-package fallback (which still
  yields the import's own name); byte-swapped (big-endian) packages throw for now; and `FPackageIndex`'s
  `WeakReference` name cache is not reproduced (Name/ToString re-resolve). All marked `TODO`.
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
  UTF-8 sub-path gate, and the AshesOfCreation/OuterWorlds2/DragonQuestXI game readers are deferred, as is the
  `Load`/`TryLoad` family (needs `IFileProvider`). `SoftObjectProperty`/`LazyObjectProperty` derive
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
  `OPTIONAL` read type. Deliberate differences: provider localization is deferred (no `IFileProvider`), so
  `Base`/`StringTableEntry` read their source/key fields but `LocalizedString` stays empty (as C# does without a
  provider) and the `UStringTable` load is skipped; the many custom-version gates
  (`FEditorObjectVersion`/`FFortniteMainBranchObjectVersion`/`FUE5ReleaseStreamObjectVersion`) aren't ported, so
  the port takes their **modern** outcome at each site (documented inline); the game-specific quirks
  (Splitgate2/DeltaForce/HonorofKingsWorld/EmbersOfTheUncrowned) are omitted; `FFormatArgumentValue`'s C# boxed
  `object Value` becomes explicit typed fields keyed by `Type`; `TextProperty` derives `FPropertyTagType`
  directly and `OptionalProperty` holds a `unique_ptr` to the inner value (null when absent, matching C#'s
  empty-string `ToString`). `OptionalProperty` passes `nullptr` for the inner tag's `InnerTypeData` (deferred
  with `FPropertyTagData`). All `TODO`.
- **Namespace-shadowing note:** introducing `CUE4Parse::UE4::Assets::Objects` (for the property types) means a
  bare `Objects::UObject::X` written *inside* `namespace …::Assets` now resolves to `Assets::Objects`, not
  `UE4::Objects`. Such references (in `IPackage`, `FAssetArchive`, `Package`, `ResolvedObject`) are now fully
  qualified as `CUE4Parse::UE4::Objects::UObject::X`.
- **Not yet ported** (arrive with their layers): the unversioned-property path + mappings provider, deferred
  object **loading** (so `ResolvedObject.Object`/`Load<T>` and the localization/`UStringTable` resolution behind
  `FText`/soft references can work), the custom-version providers (`FEditorObjectVersion` etc.; property readers
  currently assume modern), game-specific property readers (Borderlands4/OuterWorlds2/…), the named-struct table
  in `FScriptStruct` (needs Core/Math + engine structs), concrete `UExport` subclasses, `IoPackage` (IO Store),
  `IFileProvider`, and `VersionContainer` Options/MapStructTypes tables. Marked with `TODO` at their sites.

## Build

Requires CMake ≥ 3.20 and a C++20 compiler (tested with MSVC 19.38 / VS 2022, Ninja).

```powershell
# from a VS x64 developer prompt (or after running vcvars64.bat)
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

`tests/test_reader.cpp` exercises the reader primitives and the version subsystem.
