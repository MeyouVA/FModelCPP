// Ported from CUE4Parse/UE4/Wwise/Objects/AkDecisionTree.cs
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    // Nodes own their children, and the tree is built by walking breadth-first: a whole sibling row is read,
    // then each of those siblings' rows in turn. Children are held by unique_ptr because the node type is
    // still incomplete at the point the vector is declared.
    class AkDecisionTreeNode
    {
    public:
        uint32_t Key = 0;
        uint32_t AudioNodeId = 0;
        uint16_t ChildrenIndex = 0;
        uint16_t ChildrenCount = 0;
        uint16_t Weight = 0;
        uint16_t Probability = 0;
        std::vector<std::unique_ptr<AkDecisionTreeNode>> Children;

        AkDecisionTreeNode(FWwiseArchive& Ar, uint32_t countMax, int currentDepth, int maxDepth, int itemSize)
        {
            Key = Ar.Read<uint32_t>();

            const bool isAudioNode = IsAudioNode(Ar, countMax, itemSize);
            if (isAudioNode || currentDepth == maxDepth)
            {
                AudioNodeId = Ar.Read<uint32_t>();
                ChildrenCount = 0;
            }
            else
            {
                ChildrenIndex = Ar.Read<uint16_t>();
                ChildrenCount = Ar.Read<uint16_t>();
            }

            Children.resize(ChildrenCount);

            // 30..36 and >45 read the weight pair; 37..45 and <=29 do not.
            if (Ar.Version > 29 && Ar.Version <= 36)
            {
                Weight = Ar.Read<uint16_t>();
                Probability = Ar.Read<uint16_t>();
            }
            else if (Ar.Version > 45)
            {
                Weight = Ar.Read<uint16_t>();
                Probability = Ar.Read<uint16_t>();
            }
        }

    private:
        // Peeks the next four bytes: if they cannot plausibly be an (index, count) pair the node is a leaf
        // carrying an audio node id instead. The archive position is restored either way.
        static bool IsAudioNode(FWwiseArchive& Ar, uint32_t countMax, int itemSize)
        {
            const int64_t originalPosition = Ar.Position;

            const uint32_t idCh = Ar.Read<uint32_t>();
            const auto uIndex = static_cast<uint16_t>(idCh & 0xFFFF);
            const auto uCount = static_cast<uint16_t>((idCh >> 16) & 0xFFFF);

            Ar.Position = originalPosition;

            const bool isIdInvalid = uIndex > countMax || uCount > countMax;
            const bool isOverBounds = Ar.Position + static_cast<int64_t>(uCount) * itemSize > Ar.Length;

            return isIdInvalid || isOverBounds;
        }
    };

    // AkDecisionTree::SetTree
    // Used in AkDecisionTree::ResolvePath, AkDecisionTree::ResolvePathWeighted
    class AkDecisionTree
    {
    public:
        std::vector<std::unique_ptr<AkDecisionTreeNode>> Nodes;

        AkDecisionTree() = default;

        AkDecisionTree(FWwiseArchive& Ar, uint32_t treeDepth, uint32_t treeDataSize)
        {
            ParseDecisionTree(Ar, treeDataSize, treeDepth);
        }

    private:
        void ParseDecisionTree(FWwiseArchive& Ar, uint32_t size, uint32_t maxDepth)
        {
            const uint32_t itemSize = DetermineItemSize(Ar);
            const uint32_t countMax = size / itemSize;

            // C# allocates a one-element root array and only ever fills index 0 -- the root row is always
            // a single node.
            Nodes.resize(1);
            ParseTreeNode(Ar, Nodes, 1, countMax, 0, static_cast<int>(maxDepth), static_cast<int>(itemSize), nullptr);
        }

        static uint32_t DetermineItemSize(FWwiseArchive& Ar)
        {
            if (Ar.Version <= 29) return 0x08;
            if (Ar.Version <= 36) return 0x0C;
            if (Ar.Version <= 45) return 0x08;
            return 0x0C; // Default
        }

        static void ParseTreeNode(FWwiseArchive& Ar, std::vector<std::unique_ptr<AkDecisionTreeNode>>& nodes,
                                  uint32_t count, uint32_t countMax, int curDepth, int maxDepth, int itemSize,
                                  AkDecisionTreeNode* parent)
        {
            std::vector<AkDecisionTreeNode*> parsedNodes;
            parsedNodes.reserve(count);
            for (uint32_t i = 0; i < count; i++)
            {
                auto node = std::make_unique<AkDecisionTreeNode>(Ar, countMax, curDepth, maxDepth, itemSize);
                AkDecisionTreeNode* raw = node.get();
                if (parent == nullptr)
                    nodes[i] = std::move(node);
                else
                    parent->Children[i] = std::move(node);

                parsedNodes.push_back(raw);
            }

            for (AkDecisionTreeNode* node : parsedNodes)
            {
                if (node->ChildrenCount > 0)
                {
                    ParseTreeNode(Ar, nodes, node->ChildrenCount, countMax, curDepth + 1, maxDepth, itemSize, node);
                }
            }
        }
    };
}
