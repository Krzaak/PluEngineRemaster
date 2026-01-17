//
// Created by Plutex on 1/12/26.
//

#ifndef GAME_HASH_MAP_H
#define GAME_HASH_MAP_H

#include <cstddef>
#include <utility>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include "Allocators/Default.h"

namespace Plu
{
    // =========================================================================
    // GAME HASH MAP
    // =========================================================================

    template
    <
        typename TKey,
        typename TValue,
        typename THasher = DefaultHash<TKey>,
        typename TAllocator = DefaultAllocator<std::pair<TKey, TValue>>
    >
    class GameHashMap
    {
    public:
        using KeyType = TKey;
        using MappedType = TValue;
        using ValueType = std::pair<TKey, TValue>;
        using SizeType = std::size_t;
        using HasherType = THasher;
        using AllocatorType = TAllocator;

    private:
        struct Node
        {
            ValueType Data;
            Node* Next;

            template<typename... Args>
            Node(Args&&... Arguments)
                : Data(static_cast<Args&&>(Arguments)...), Next(nullptr) {}
        };

        using NodeAllocator = DefaultAllocator<Node>;

        Node** Buckets;
        SizeType BucketCount;
        SizeType ElementCount;
        HasherType Hasher;
        NodeAllocator NodeAlloc;
        AllocatorType ValueAlloc;

        static constexpr float LoadFactorThreshold = 0.75f;
        static constexpr SizeType MinBucketCount = 8;

        SizeType GetBucketIndex(const TKey& Key) const noexcept
        {
            return Hasher(Key) % BucketCount;
        }

        void DestroyAllNodes() noexcept
        {
            if (!Buckets)
                return;

            for (SizeType I = 0; I < BucketCount; ++I)
            {
                Node* Current = Buckets[I];
                while (Current)
                {
                    Node* Next = Current->Next;
                    NodeAlloc.Destroy(Current);
                    NodeAlloc.Deallocate(Current, 1);
                    Current = Next;
                }
            }
        }

        void RehashInternal(SizeType NewBucketCount)
        {
            if (NewBucketCount < MinBucketCount)
                NewBucketCount = MinBucketCount;

            Node** NewBuckets = static_cast<Node**>(::operator new(NewBucketCount * sizeof(Node*)));

            for (SizeType I = 0; I < NewBucketCount; ++I)
                NewBuckets[I] = nullptr;

            // Rehash all existing nodes
            for (SizeType I = 0; I < BucketCount; ++I)
            {
                Node* Current = Buckets[I];
                while (Current)
                {
                    Node* Next = Current->Next;
                    SizeType NewIndex = Hasher(Current->Data.first) % NewBucketCount;
                    Current->Next = NewBuckets[NewIndex];
                    NewBuckets[NewIndex] = Current;
                    Current = Next;
                }
            }

            ::operator delete(Buckets);
            Buckets = NewBuckets;
            BucketCount = NewBucketCount;
        }

        void CheckLoadFactorAndRehash()
        {
            float CurrentLoadFactor = static_cast<float>(ElementCount) / static_cast<float>(BucketCount);
            if (CurrentLoadFactor > LoadFactorThreshold)
            {
                RehashInternal(BucketCount * 2);
            }
        }

    public:
        // =====================================================================
        // ITERATOR IMPLEMENTATION
        // =====================================================================

        class Iterator
        {
            friend class GameHashMap;

            Node** BucketsPtr;
            SizeType BucketCountVal;
            SizeType CurrentBucket;
            Node* CurrentNode;

            void AdvanceToNextValid()
            {
                while (!CurrentNode && CurrentBucket < BucketCountVal)
                {
                    ++CurrentBucket;
                    if (CurrentBucket < BucketCountVal)
                        CurrentNode = BucketsPtr[CurrentBucket];
                }
            }

        public:
            Iterator(Node** Buckets, SizeType BucketCount, SizeType Bucket, Node* Node)
                : BucketsPtr(Buckets), BucketCountVal(BucketCount), CurrentBucket(Bucket), CurrentNode(Node)
            {
                if (!CurrentNode)
                    AdvanceToNextValid();
            }

            ValueType& operator*() const { return CurrentNode->Data; }
            ValueType* operator->() const { return &CurrentNode->Data; }

            Iterator& operator++()
            {
                if (CurrentNode)
                {
                    CurrentNode = CurrentNode->Next;
                    if (!CurrentNode)
                        AdvanceToNextValid();
                }
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator Temp = *this;
                ++(*this);
                return Temp;
            }

            bool operator==(const Iterator& Other) const
            {
                return CurrentNode == Other.CurrentNode && CurrentBucket == Other.CurrentBucket;
            }

            bool operator!=(const Iterator& Other) const
            {
                return !(*this == Other);
            }
        };

        class ConstIterator
        {
            friend class GameHashMap;

            Node* const* BucketsPtr;
            SizeType BucketCountVal;
            SizeType CurrentBucket;
            const Node* CurrentNode;

            void AdvanceToNextValid()
            {
                while (!CurrentNode && CurrentBucket < BucketCountVal)
                {
                    ++CurrentBucket;
                    if (CurrentBucket < BucketCountVal)
                        CurrentNode = BucketsPtr[CurrentBucket];
                }
            }

        public:
            ConstIterator(Node* const* Buckets, SizeType BucketCount, SizeType Bucket, const Node* Node)
                : BucketsPtr(Buckets), BucketCountVal(BucketCount), CurrentBucket(Bucket), CurrentNode(Node)
            {
                if (!CurrentNode)
                    AdvanceToNextValid();
            }

            const ValueType& operator*() const { return CurrentNode->Data; }
            const ValueType* operator->() const { return &CurrentNode->Data; }

            ConstIterator& operator++()
            {
                if (CurrentNode)
                {
                    CurrentNode = CurrentNode->Next;
                    if (!CurrentNode)
                        AdvanceToNextValid();
                }
                return *this;
            }

            ConstIterator operator++(int)
            {
                ConstIterator Temp = *this;
                ++(*this);
                return Temp;
            }

            bool operator==(const ConstIterator& Other) const
            {
                return CurrentNode == Other.CurrentNode && CurrentBucket == Other.CurrentBucket;
            }

            bool operator!=(const ConstIterator& Other) const
            {
                return !(*this == Other);
            }
        };

        // =====================================================================
        // CONSTRUCTORS & DESTRUCTOR
        // =====================================================================

        GameHashMap()
            : Buckets(nullptr), BucketCount(MinBucketCount), ElementCount(0)
        {
            Buckets = static_cast<Node**>(::operator new(BucketCount * sizeof(Node*)));
            for (SizeType I = 0; I < BucketCount; ++I)
                Buckets[I] = nullptr;
        }

        explicit GameHashMap(SizeType InitialBucketCount)
            : Buckets(nullptr), BucketCount(InitialBucketCount < MinBucketCount ? MinBucketCount : InitialBucketCount), ElementCount(0)
        {
            Buckets = static_cast<Node**>(::operator new(BucketCount * sizeof(Node*)));
            for (SizeType I = 0; I < BucketCount; ++I)
                Buckets[I] = nullptr;
        }

        ~GameHashMap() noexcept
        {
            Clear();
            if (Buckets)
                ::operator delete(Buckets);
        }

        GameHashMap(const GameHashMap& Other)
            : Buckets(nullptr), BucketCount(Other.BucketCount), ElementCount(0)
        {
            Buckets = static_cast<Node**>(::operator new(BucketCount * sizeof(Node*)));
            for (SizeType I = 0; I < BucketCount; ++I)
                Buckets[I] = nullptr;

            for (const auto& [Key, Value] : Other)
                Insert(Key, Value);
        }

        GameHashMap(GameHashMap&& Other) noexcept
            : Buckets(Other.Buckets), BucketCount(Other.BucketCount), ElementCount(Other.ElementCount)
        {
            Other.Buckets = nullptr;
            Other.BucketCount = 0;
            Other.ElementCount = 0;
        }

        GameHashMap& operator=(const GameHashMap& Other)
        {
            if (this != &Other)
            {
                Clear();
                Reserve(Other.ElementCount);
                for (const auto& [Key, Value] : Other)
                    Insert(Key, Value);
            }
            return *this;
        }

        GameHashMap& operator=(GameHashMap&& Other) noexcept
        {
            if (this != &Other)
            {
                Clear();
                if (Buckets)
                    ::operator delete(Buckets);

                Buckets = Other.Buckets;
                BucketCount = Other.BucketCount;
                ElementCount = Other.ElementCount;

                Other.Buckets = nullptr;
                Other.BucketCount = 0;
                Other.ElementCount = 0;
            }
            return *this;
        }

        // =====================================================================
        // CORE FUNCTIONALITY
        // =====================================================================

        bool Insert(const TKey& Key, const TValue& Value)
        {
            SizeType Index = GetBucketIndex(Key);
            Node* Current = Buckets[Index];

            while (Current)
            {
                if (Current->Data.first == Key)
                    return false; // Key already exists
                Current = Current->Next;
            }

            Node* NewNode = NodeAlloc.Allocate(1);
            NodeAlloc.Construct(NewNode, Key, Value);
            NewNode->Next = Buckets[Index];
            Buckets[Index] = NewNode;
            ++ElementCount;

            CheckLoadFactorAndRehash();
            return true;
        }

        bool Insert(TKey&& Key, TValue&& Value)
        {
            SizeType Index = GetBucketIndex(Key);
            Node* Current = Buckets[Index];

            while (Current)
            {
                if (Current->Data.first == Key)
                    return false;
                Current = Current->Next;
            }

            Node* NewNode = NodeAlloc.Allocate(1);
            NodeAlloc.Construct(NewNode, static_cast<TKey&&>(Key), static_cast<TValue&&>(Value));
            NewNode->Next = Buckets[Index];
            Buckets[Index] = NewNode;
            ++ElementCount;

            CheckLoadFactorAndRehash();
            return true;
        }

        template<typename... Args>
        bool Emplace(const TKey& Key, Args&&... Arguments)
        {
            SizeType Index = GetBucketIndex(Key);
            Node* Current = Buckets[Index];

            while (Current)
            {
                if (Current->Data.first == Key)
                    return false;
                Current = Current->Next;
            }

            Node* NewNode = NodeAlloc.Allocate(1);
            NodeAlloc.Construct(NewNode,
                std::piecewise_construct,
                std::forward_as_tuple(Key),
                std::forward_as_tuple(static_cast<Args&&>(Arguments)...));
            NewNode->Next = Buckets[Index];
            Buckets[Index] = NewNode;
            ++ElementCount;

            CheckLoadFactorAndRehash();
            return true;
        }

        bool Remove(const TKey& Key) noexcept
        {
            SizeType Index = GetBucketIndex(Key);
            Node* Current = Buckets[Index];
            Node* Previous = nullptr;

            while (Current)
            {
                if (Current->Data.first == Key)
                {
                    if (Previous)
                        Previous->Next = Current->Next;
                    else
                        Buckets[Index] = Current->Next;

                    NodeAlloc.Destroy(Current);
                    NodeAlloc.Deallocate(Current, 1);
                    --ElementCount;
                    return true;
                }
                Previous = Current;
                Current = Current->Next;
            }

            return false;
        }

        TValue* Find(const TKey& Key) noexcept
        {
            SizeType Index = GetBucketIndex(Key);
            Node* Current = Buckets[Index];

            while (Current)
            {
                if (Current->Data.first == Key)
                    return &Current->Data.second;
                Current = Current->Next;
            }

            return nullptr;
        }

        const TValue* Find(const TKey& Key) const noexcept
        {
            SizeType Index = GetBucketIndex(Key);
            Node* Current = Buckets[Index];

            while (Current)
            {
                if (Current->Data.first == Key)
                    return &Current->Data.second;
                Current = Current->Next;
            }

            return nullptr;
        }

        bool Contains(const TKey& Key) const noexcept
        {
            return Find(Key) != nullptr;
        }

        TValue& operator[](const TKey& Key)
        {
            SizeType Index = GetBucketIndex(Key);
            Node* Current = Buckets[Index];

            while (Current)
            {
                if (Current->Data.first == Key)
                    return Current->Data.second;
                Current = Current->Next;
            }

            // Insert new element with default value
            Node* NewNode = NodeAlloc.Allocate(1);
            NodeAlloc.Construct(NewNode, Key, TValue{});
            NewNode->Next = Buckets[Index];
            Buckets[Index] = NewNode;
            ++ElementCount;

            CheckLoadFactorAndRehash();
            return NewNode->Data.second;
        }

        void Clear() noexcept
        {
            DestroyAllNodes();
            for (SizeType I = 0; I < BucketCount; ++I)
                Buckets[I] = nullptr;
            ElementCount = 0;
        }

        SizeType Size() const noexcept { return ElementCount; }
        bool IsEmpty() const noexcept { return ElementCount == 0; }

        void Reserve(SizeType Count)
        {
            SizeType RequiredBuckets = static_cast<SizeType>(Count / LoadFactorThreshold) + 1;
            if (RequiredBuckets > BucketCount)
                RehashInternal(RequiredBuckets);
        }

        void Rehash(SizeType Count)
        {
            RehashInternal(Count);
        }

        // =====================================================================
        // ITERATORS
        // =====================================================================

        Iterator begin() noexcept
        {
            for (SizeType I = 0; I < BucketCount; ++I)
            {
                if (Buckets[I])
                    return Iterator(Buckets, BucketCount, I, Buckets[I]);
            }
            return end();
        }

        Iterator end() noexcept
        {
            return Iterator(Buckets, BucketCount, BucketCount, nullptr);
        }

        ConstIterator begin() const noexcept
        {
            for (SizeType I = 0; I < BucketCount; ++I)
            {
                if (Buckets[I])
                    return ConstIterator(Buckets, BucketCount, I, Buckets[I]);
            }
            return end();
        }

        ConstIterator end() const noexcept
        {
            return ConstIterator(Buckets, BucketCount, BucketCount, nullptr);
        }
    };

} // namespace Plu

#endif
