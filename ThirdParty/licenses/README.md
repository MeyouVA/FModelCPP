# Third-party binary licences

Every binary in `../native/` and where its licence comes from. BSD, MIT, ISC and zlib all require the
copyright notice to accompany **binary** redistribution, which is what these files are for.

| Binary | Project | Version (from the file's own version resource) | Licence | Text |
| --- | --- | --- | --- | --- |
| `libzstd.dll`, `zstd.dll` | Zstandard — Meta Platforms, Inc. | 1.5.7 | BSD-3-Clause (dual-licensed BSD / GPLv2) | [zstd-LICENSE.txt](zstd-LICENSE.txt) |
| `brotlicommon.dll`, `brotlidec.dll`, `brotlienc.dll` | Brotli — the Brotli Authors | — | MIT | [brotli-LICENSE.txt](brotli-LICENSE.txt) |
| `zlib-ng2.dll` | zlib-ng | 2.3.2 | zlib | [zlib-ng-LICENSE.txt](zlib-ng-LICENSE.txt) |
| `Detex.dll` | detex — Harm Hanemaaijer | — | ISC | [detex-LICENSE.txt](detex-LICENSE.txt) |
| `crunch.dll` | crunch / crnlib — Richard Geldreich, Jr. and Binomial LLC | — | zlib | [crunch-LICENSE.txt](crunch-LICENSE.txt) |
| `tegra_swizzle_x64.dll` | tegra_swizzle — SMG (ScanMountGoat) | — | MIT | [tegra_swizzle-LICENSE.txt](tegra_swizzle-LICENSE.txt) |
| `oodle-data-shared.dll`, `oo2core_9_win64.dll` | Oodle Data — RAD Game Tools / Epic Games | — | **proprietary — not redistributable** | none; git-ignored, each user supplies their own |

`Detex.dll`, `crunch.dll` and `tegra_swizzle_x64.dll` are byte-identical to the copies the upstream
CUE4Parse repository ships in `CUE4Parse-Conversion/Resources/`.

Where a licence text was not available locally the licence type and copyright line were taken from the
project's own `LICENSE` file at the URL named in the header of each text file, with the canonical
boilerplate for that licence as the body. `crunch-LICENSE.txt` is reproduced from the CUE4Parse `NOTICE`,
which ships the same binary. Versions are read out of each DLL's version resource; a dash means the file
carries no version resource.

Removed on purpose: `msys-lz4-1.dll`. It was an MSYS2 build that imports `msys-2.0.dll` — the Cygwin-derived
runtime, which is LGPLv3 — the dependency was not present so it could not have loaded anyway, and this port
implements LZ4 in-tree.
