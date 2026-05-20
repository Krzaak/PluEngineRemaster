//
// Created by Plutex on 1/12/26.
//

#ifndef PLUENGINE_STRINGHASHER_H
#define PLUENGINE_STRINGHASHER_H

#include <cstddef>
#include "Default.h"

namespace Plu
{
    // Forward declaration dla BasicString
    template<typename CharT, typename Allocator>
    class BasicString;

    template<typename CharT, typename Allocator>
    class BasicPath;

    // ============================================================================
    // STRING HASH SPECIALIZATIONS
    // ============================================================================

    // Specialization for BasicString<CharT, Allocator>
    template<typename CharT, typename Allocator>
    struct DefaultHash<BasicString<CharT, Allocator>> {
        std::size_t operator()(const BasicString<CharT, Allocator>& str) const noexcept {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(str.CStr());
            std::size_t len = str.Length() * sizeof(CharT);   // ← całe bajty
            std::size_t hash = 14695981039346656037ULL;

            for (std::size_t i = 0; i < len; ++i) {
                hash ^= p[i];
                hash *= 1099511628211ULL;
            }
            return hash;
        }
    };

    template<typename CharT, typename Allocator>
    struct DefaultHash<BasicPath<CharT, Allocator>> {
        std::size_t operator()(const BasicPath<CharT, Allocator>& str) const noexcept {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(str.ToString().CStr());
            std::size_t len = str.ToString().Length() * sizeof(CharT);   // ← całe bajty
            std::size_t hash = 14695981039346656037ULL;

            for (std::size_t i = 0; i < len; ++i) {
                hash ^= p[i];
                hash *= 1099511628211ULL;
            }
            return hash;
        }
    };

    // Specjalizacja dla const char* (C-string)
    template<>
    struct DefaultHash<const char*> {
        std::size_t operator()(const char* str) const noexcept {
            if (!str) return 0;

            std::size_t hash = 14695981039346656037ULL;
            while (*str) {
                hash ^= static_cast<unsigned char>(*str);
                hash *= 1099511628211ULL;
                ++str;
            }
            return hash;
        }
    };

    // Specjalizacja dla const wchar_t* (wide C-string)
    template<>
    struct DefaultHash<const wchar_t*> {
        std::size_t operator()(const wchar_t* str) const noexcept {
            if (!str) return 0;

            std::size_t hash = 14695981039346656037ULL;
            while (*str) {
                const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str);
                for (std::size_t i = 0; i < sizeof(wchar_t); ++i) {
                    hash ^= bytes[i];
                    hash *= 1099511628211ULL;
                }
                ++str;
            }
            return hash;
        }
    };
}

#endif //PLUENGINE_STRINGHASHER_H