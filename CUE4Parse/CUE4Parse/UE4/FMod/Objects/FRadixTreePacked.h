// Ported from CUE4Parse/UE4/FMod/Objects/FRadixTreePacked.cs (+ the FPackedNode partial in FPackedNode.cs)
// A packed radix tree mapping event FModGuids to their human-readable string paths. C# splits this across
// two files via `partial class`; the port folds FPackedNode back in as a nested struct (its HasString
// helper reads the outer Sentinel24 constant, just as in C#).
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "FModGuid.h"
#include "FUInt24.h"
#include "../FModReader.h"
#include "../Enums/EStringTableType.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    class FRadixTreePacked
    {
    public:
        static constexpr int Sentinel24 = 0xFFFFFF;

        struct FPackedNode
        {
            uint32_t KeyInfo = 0;
            uint32_t ChildInfo = 0;

            FPackedNode() = default;
            explicit FPackedNode(Readers::FArchive& Ar)
            {
                KeyInfo = Ar.Read<uint32_t>();
                ChildInfo = Ar.Read<uint32_t>();
            }

            int StringOffset() const { return static_cast<int>(KeyInfo & 0x00FFFFFF); }
            bool HasString() const { return StringOffset() != Sentinel24; }
        };

        std::vector<FPackedNode> Nodes;
        std::vector<FModGuid> Guids;
        std::vector<uint8_t> StringBlob;
        std::vector<FUInt24> LeafIndices;
        std::vector<FUInt24> ParentIndices;

        FRadixTreePacked(Readers::FArchive& Ar, Enums::EStringTableType type)
        {
            Nodes = FModReader::ReadElemListImp<FPackedNode>(Ar);
            Guids = FModReader::ReadElemListImp<FModGuid>(Ar);
            StringBlob = ReadByteArray(Ar);

            if (type == Enums::EStringTableType::StringTable_RadixTree_24Bit)
            {
                LeafIndices = FModReader::ReadSimpleArray24(Ar);
                ParentIndices = FModReader::ReadSimpleArray24(Ar);
            }

            for (size_t i = 0; i < Guids.size(); i++)
                _guidIndex[Guids[i]] = static_cast<int>(i);
        }

        bool TryGetString(const FModGuid& guid, std::string& path) const
        {
            auto it = _guidIndex.find(guid);
            if (it != _guidIndex.end())
            {
                TryGetStringByIndex(it->second, path);
                return true;
            }
            path.clear();
            return false;
        }

        bool TryGetStringByIndex(int guidIndex, std::string& path, size_t maxLen = static_cast<size_t>(-1)) const
        {
            path.clear();

            if (guidIndex < 0 || guidIndex >= static_cast<int>(Guids.size()))
                return false;
            if (LeafIndices.size() != Guids.size())
                return false;
            if (ParentIndices.size() != Nodes.size())
                return false;

            uint32_t node = LeafIndices[guidIndex].Value;
            if (node == Sentinel24)
            {
                path.clear();
                return true;
            }

            std::vector<std::string> segments;
            int guard = 0;

            while (node != Sentinel24)
            {
                if (node >= Nodes.size())
                    break;

                const FPackedNode& n = Nodes[node];
                if (n.HasString())
                    segments.push_back(ReadCString(n.StringOffset()));

                if (node >= ParentIndices.size())
                    break;

                node = ParentIndices[node].Value;

                if (++guard > 100000)
                    throw std::runtime_error("Parent chain too long");
            }

            std::string full;
            for (auto it = segments.rbegin(); it != segments.rend(); ++it)
                full += *it;

            if (full.size() + 1 > maxLen)
            {
                path = full.size() >= maxLen - 1 ? full.substr(0, maxLen >= 1 ? maxLen - 1 : 0) : full;
                return true; // It's truncated but we ball
            }

            path = full;
            return true;
        }

    private:
        std::unordered_map<FModGuid, int> _guidIndex;

        static std::vector<uint8_t> ReadByteArray(Readers::FArchive& Ar)
        {
            uint32_t count = FModReader::ReadX16(Ar);
            return Ar.ReadBytes(static_cast<int>(count));
        }

        std::string ReadCString(int offset) const
        {
            if (offset < 0 || offset >= static_cast<int>(StringBlob.size()))
                return std::string();

            int end = offset;
            while (end < static_cast<int>(StringBlob.size()) && StringBlob[end] != 0)
                end++;

            return std::string(StringBlob.begin() + offset, StringBlob.begin() + end);
        }
    };
}
