// Runtime lookup for the native codec libraries (Oodle, Zstd) the port loads on demand.
//
// C# has no equivalent: `DllImport`/`NativeLibrary.Load` lean on the .NET probing rules, and FModel
// downloads Oodle and Zlib-ng at runtime into its own data directory. The C++ port needs an explicit
// search order instead, and it must not bake anyone's machine paths into the source — so nothing here is
// absolute. Candidates are tried in this order:
//
//   1. the caller's explicit path, if it gave one
//   2. $FMODELCPP_NATIVE_DIR/<name>            (an override for CI or an unusual install)
//   3. <directory of the running executable>/<name>
//   4. <exe dir>/ThirdParty/native/<platform>/<name>, and the same walking up to four parent
//      directories — which is what finds the in-tree ThirdParty folder from a build tree
//   5. the bare name, leaving it to the OS loader (PATH / LD_LIBRARY_PATH / rpath)
//
// Drop a DLL in `ThirdParty/native/win-x64` (or `.so` in `linux-x64`) and it is found with no
// configuration; CMake also copies whatever is there next to the built binaries.
#pragma once

#include <string>
#include <vector>

namespace CUE4Parse::Compression
{
    // The platform subdirectory used under ThirdParty/native — "win-x64" or "linux-x64".
    const char* NativePlatformDir();

    // Absolute path of the directory holding the running executable, or "" if it cannot be determined.
    std::string ExecutableDirectory();

    // Every path worth trying for `libraryName`, in search order. `explicitPath` (may be empty) always
    // comes first.
    std::vector<std::string> NativeLibraryCandidates(const std::string& libraryName,
                                                     const std::string& explicitPath = "");

    // dlopen / LoadLibrary. On Windows this uses LOAD_WITH_ALTERED_SEARCH_PATH so that a library loaded by
    // full path can find ITS OWN dependencies beside it — brotlidec.dll needs brotlicommon.dll, and a plain
    // LoadLibraryA would look for that next to the .exe and on PATH instead, then fail for no visible reason.
    void* LoadNativeLibrary(const std::string& path);

    // dlsym / GetProcAddress.
    void* GetNativeSymbol(void* library, const char* symbol);
}
