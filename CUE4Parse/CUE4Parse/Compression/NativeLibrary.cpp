// See NativeLibrary.h.
#include "NativeLibrary.h"

#include <cstdlib>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <dlfcn.h>
#  include <limits.h>
#  include <unistd.h>
#endif

namespace CUE4Parse::Compression
{
    namespace
    {
        constexpr char SEP =
#if defined(_WIN32)
            '\\';
#else
            '/';
#endif

        std::string Join(const std::string& dir, const std::string& leaf)
        {
            if (dir.empty()) return leaf;
            if (dir.back() == '/' || dir.back() == '\\') return dir + leaf;
            return dir + SEP + leaf;
        }

        std::string ParentOf(const std::string& path)
        {
            const size_t slash = path.find_last_of("/\\");
            return slash == std::string::npos ? std::string() : path.substr(0, slash);
        }

        std::string EnvVar(const char* name)
        {
#if defined(_MSC_VER)
            // getenv is deprecated under MSVC; _dupenv_s is the sanctioned replacement.
            char* value = nullptr;
            size_t len = 0;
            if (_dupenv_s(&value, &len, name) != 0 || value == nullptr) return {};
            std::string result(value);
            std::free(value);
            return result;
#else
            const char* value = std::getenv(name);
            return value != nullptr ? std::string(value) : std::string();
#endif
        }
    }

    const char* NativePlatformDir()
    {
#if defined(_WIN32)
        return "win-x64";
#else
        return "linux-x64";
#endif
    }

    std::string ExecutableDirectory()
    {
#if defined(_WIN32)
        char buffer[MAX_PATH] = {};
        const DWORD n = ::GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        if (n == 0 || n >= MAX_PATH) return {};
        return ParentOf(std::string(buffer, n));
#else
        char buffer[PATH_MAX] = {};
        const ssize_t n = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (n <= 0) return {};
        return ParentOf(std::string(buffer, static_cast<size_t>(n)));
#endif
    }

    std::vector<std::string> NativeLibraryCandidates(const std::string& libraryName, const std::string& explicitPath)
    {
        std::vector<std::string> candidates;
        if (!explicitPath.empty()) candidates.push_back(explicitPath);

        const std::string override_ = EnvVar("FMODELCPP_NATIVE_DIR");
        if (!override_.empty())
        {
            candidates.push_back(Join(override_, libraryName));
            candidates.push_back(Join(Join(override_, NativePlatformDir()), libraryName));
        }

        const std::string exeDir = ExecutableDirectory();
        if (!exeDir.empty())
        {
            candidates.push_back(Join(exeDir, libraryName));

            // Walk up from the executable looking for the in-tree ThirdParty folder. Four levels covers a
            // CMake build tree (build/tests/<exe> and multi-config variants) without hunting the whole disk.
            std::string dir = exeDir;
            for (int i = 0; i < 5 && !dir.empty(); ++i)
            {
                candidates.push_back(Join(Join(Join(Join(dir, "ThirdParty"), "native"), NativePlatformDir()), libraryName));
                dir = ParentOf(dir);
            }
        }

        // Last resort: hand the bare name to the OS loader.
        candidates.push_back(libraryName);
        return candidates;
    }

    void* LoadNativeLibrary(const std::string& path)
    {
        if (path.empty()) return nullptr;
#if defined(_WIN32)
        // A bare name has no directory for the loader to alter, so the flag is only meaningful (and only
        // legal) for a full path.
        const bool hasDirectory = path.find_first_of("/\\") != std::string::npos;
        const DWORD flags = hasDirectory ? LOAD_WITH_ALTERED_SEARCH_PATH : 0;
        return reinterpret_cast<void*>(::LoadLibraryExA(path.c_str(), nullptr, flags));
#else
        return ::dlopen(path.c_str(), RTLD_NOW);
#endif
    }

    void* GetNativeSymbol(void* library, const char* symbol)
    {
        if (library == nullptr) return nullptr;
#if defined(_WIN32)
        return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(library), symbol));
#else
        return ::dlsym(library, symbol);
#endif
    }
}
