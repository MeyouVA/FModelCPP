// Ported from CUE4Parse/UE4/Plugins/UPluginManifest.cs
// The two JSON shapes LoadVirtualPaths reads: a .upluginmanifest (a list of plugin files, each with an
// inlined descriptor) and a .uplugin (one descriptor on its own).
//
// Deliberate differences from C#:
//   * C# gets its deserialization from Newtonsoft's `[JsonProperty]` reflection; C++ has none, so each type
//     carries a FromJson over the Utils::Json reader. Only CanContainContent is read out of a descriptor,
//     exactly as in C# — the class declares that one property and nothing else.
//   * `Amount` is a DebuggerDisplay convenience in C#; kept as Amount() since it costs nothing.
#pragma once

#include <string>
#include <vector>

#include "../../Utils/Json.h"

namespace CUE4Parse::UE4::Plugins
{
    class UPluginDescriptor
    {
    public:
        bool CanContainContent = false;

        static UPluginDescriptor FromJson(const Utils::Json::JValue& json)
        {
            UPluginDescriptor descriptor;
            descriptor.CanContainContent = json["CanContainContent"].AsBool(false);
            return descriptor;
        }
    };

    class UPluginContents
    {
    public:
        std::string File;
        UPluginDescriptor Descriptor;

        static UPluginContents FromJson(const Utils::Json::JValue& json)
        {
            UPluginContents contents;
            contents.File = json["File"].AsString();
            contents.Descriptor = UPluginDescriptor::FromJson(json["Descriptor"]);
            return contents;
        }
    };

    class UPluginManifest
    {
    public:
        std::vector<UPluginContents> Contents;

        int Amount() const { return static_cast<int>(Contents.size()); }

        static UPluginManifest FromJson(const Utils::Json::JValue& json)
        {
            UPluginManifest manifest;
            const Utils::Json::JValue& contents = json["Contents"];
            manifest.Contents.reserve(contents.Count());
            for (const Utils::Json::JValue& entry : contents.Values())
                manifest.Contents.push_back(UPluginContents::FromJson(entry));
            return manifest;
        }
    };
}
