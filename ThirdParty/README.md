# ThirdParty native libraries

Drop native codec libraries here and the port finds them with no configuration:

```
ThirdParty/native/win-x64/     *.dll
ThirdParty/native/linux-x64/   *.so
```

CMake copies whatever is in the folder for the current platform next to the built binaries, and
`CUE4Parse/Compression/NativeLibrary.{h,cpp}` also probes this folder directly (walking up from the
executable), so it works whether you run from the build tree or from an install. Set
`FMODELCPP_NATIVE_DIR` to override the location. **No absolute path is compiled into the source.**

## What goes here

| File | Needed for | Where the C# project gets it |
| --- | --- | --- |
| `libzstd.dll` (or `zstd.dll`) | Zstd-compressed `.usmap` mappings and Zstd pak/IO Store blocks | C# uses the managed `ZstdSharp` package, so it ships no DLL |
| `oodle-data-shared.dll` (or the older `oo2core_9_win64.dll`) | Oodle-compressed pak / IO Store blocks — most shipped UE4.20+ games | FModel downloads it at runtime |
| `brotlidec.dll` + `brotlicommon.dll` | Brotli-compressed `.usmap` mappings | C# uses `System.IO.Compression.BrotliStream` from the BCL, so it ships no DLL |

Linux equivalents: `libzstd.so`, `liboodle-data-shared.so`, `libbrotlidec.so`.

`brotlidec` needs `brotlicommon` beside it; that works because libraries are loaded by full path with the
altered search path (see `LoadNativeLibrary`), so a dependency next to the DLL resolves.

Neither is required to build or to run the test suite: `tests/test_zstd.cpp` reports **SKIPPED** when no
libzstd is present, and the Oodle path simply stays unregistered until `OodleHelper::Initialize` succeeds.

## What is in `native/win-x64` right now

| File | Status |
| --- | --- |
| `libzstd.dll`, `zstd.dll` | **used** — either name is tried, `libzstd.dll` first |
| `oodle-data-shared.dll`, `oo2core_9_win64.dll` | **used** — current name tried first, then the old one. Verified: 10 Oodle-compressed Fortnite `.usmap` files that previously failed now parse |
| `Detex.dll`, `crunch.dll`, `tegra_swizzle_x64.dll` | not used yet — texture decode, which lives in the still-unported `CUE4Parse-Conversion` tree. All three are byte-identical to the C# repo's copies, so they are ready when that slice lands |
| `brotlidec.dll`, `brotlicommon.dll` | **used** — the Brotli-compressed Fortnite `.usmap` that previously failed now parses |
| `brotlienc.dll` | unused — decompression only; harmless |
| `zlib-ng2.dll` | unused — this port implements inflate in-tree |

With all three codec libraries present, **31 of 32 real `.usmap` files parse** — the only failure left on
this machine is a zero-byte file.

## Not needed yet

These are checked into the C# repo under `CUE4Parse-Conversion/Resources/`, and will matter only once the
texture *decoding* tree is ported (it lives in the separate `CUE4Parse-Conversion` assembly):

`Detex.dll` · `crunch.dll` · `tegra_swizzle_x64.dll`

Likewise `CUE4Parse-Natives.dll` (ACL animation decompression), `zlib-ng2.dll` (the port has an in-tree
inflate) and `ProSpiDecryptor.dll` (ProSpi AES, unported).

## Licensing

Every binary here is catalogued in [licenses/README.md](licenses/README.md) with its project, version,
licence and licence text. All of them are permissive (BSD-3 / MIT / ISC / zlib) — but those licences
require the copyright notice to accompany binary redistribution, which is what `licenses/` is for.

**The exception is Oodle** (`oodle-data-shared.dll`, `oo2core_9_win64.dll`): proprietary RAD Game Tools
code, not redistributable. Both names are in the repository `.gitignore`, and each user supplies their own
copy in this folder.

`msys-lz4-1.dll` was removed rather than shipped: it was an MSYS2 build importing the Cygwin-derived
`msys-2.0.dll` (LGPLv3), it could not load without that runtime, and the port implements LZ4 in-tree.
