//
// Created by Plutex on 1/12/26.
//

#ifndef PLUENGINE_DEFAULT_H
#define PLUENGINE_DEFAULT_H
#include <cstddef>
#include <cstdint>

namespace Plu
{
	// ============================================================================
    // DEFAULT HASH FUNCTORS
    // ============================================================================

    template<typename T>
    struct DefaultHash {
        std::size_t operator()(const T& key) const noexcept {
            // FNV-1a hash for general types (bitwise)
            const unsigned char* data = reinterpret_cast<const unsigned char*>(&key);
            std::size_t hash = 14695981039346656037ULL;
            for (std::size_t i = 0; i < sizeof(T); ++i) {
                hash ^= data[i];
                hash *= 1099511628211ULL;
            }
            return hash;
        }
    };

    // Specialization for integers (identity-like but mixed)
    template<>
    struct DefaultHash<uint32_t> {
        std::size_t operator()(uint32_t key) const noexcept {
            // MurmurHash3 finalizer
            key ^= key >> 16;
            key *= 0x85ebca6b;
            key ^= key >> 13;
            key *= 0xc2b2ae35;
            key ^= key >> 16;
            return key;
        }
    };

    template<>
    struct DefaultHash<uint64_t> {
        std::size_t operator()(uint64_t key) const noexcept {
            // Splitmix64
            key ^= key >> 30;
            key *= 0xbf58476d1ce4e5b9ULL;
            key ^= key >> 27;
            key *= 0x94d049bb133111ebULL;
            key ^= key >> 31;
            return key;
        }
    };

    template<>
    struct DefaultHash<int32_t> {
        std::size_t operator()(int32_t key) const noexcept {
            return DefaultHash<uint32_t>{}(static_cast<uint32_t>(key));
        }
    };

    template<>
    struct DefaultHash<int64_t> {
        std::size_t operator()(int64_t key) const noexcept {
            return DefaultHash<uint64_t>{}(static_cast<uint64_t>(key));
        }
    };

    // Pointer specialization
    template<typename T>
    struct DefaultHash<T*> {
        std::size_t operator()(T* ptr) const noexcept {
            return DefaultHash<uint64_t>{}(reinterpret_cast<uint64_t>(ptr));
        }
    };

    template<>
    struct DefaultHash<long long>
    {
        std::size_t operator()(long long Key) const noexcept
        {
            std::size_t Hash = static_cast<std::size_t>(Key);
            Hash ^= (Hash >> 33);
            Hash *= 0xff51afd7ed558ccdULL;
            Hash ^= (Hash >> 33);
            Hash *= 0xc4ceb9fe1a85ec53ULL;
            Hash ^= (Hash >> 33);
            return Hash;
        }
    };
}

#endif //PLUENGINE_DEFAULT_H