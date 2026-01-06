#pragma once
#include <cstring>
#include <cstddef>
#include <algorithm>
#include <utility>
#include <ostream>
#include <cstdio>
#include <cctype>
#include <vector>
#include "Allocators/Default.h"

namespace Plu
{
    // Traits dla różnych typów znaków
    template<typename CharT>
    struct CharTraits {
        static size_t Length(const CharT* str) {
            if (!str) return 0;
            size_t len = 0;
            while (str[len] != CharT(0)) ++len;
            return len;
        }
        
        static int Compare(const CharT* s1, const CharT* s2, size_t n) {
            for (size_t i = 0; i < n; ++i) {
                if (s1[i] < s2[i]) return -1;
                if (s1[i] > s2[i]) return 1;
            }
            return 0;
        }
        
        static const CharT* Find(const CharT* str, size_t n, CharT c) {
            for (size_t i = 0; i < n; ++i) {
                if (str[i] == c) return str + i;
            }
            return nullptr;
        }
        
        static CharT* Copy(CharT* dst, const CharT* src, size_t n) {
            for (size_t i = 0; i < n; ++i) {
                dst[i] = src[i];
            }
            return dst;
        }
        
        static CharT* Move(CharT* dst, const CharT* src, size_t n) {
            if (dst < src) {
                return Copy(dst, src, n);
            } else if (dst > src) {
                for (size_t i = n; i > 0; --i) {
                    dst[i - 1] = src[i - 1];
                }
            }
            return dst;
        }
        
        static CharT* Set(CharT* dst, CharT c, size_t n) {
            for (size_t i = 0; i < n; ++i) {
                dst[i] = c;
            }
            return dst;
        }
    };

    // Specjalizacja dla char - używa szybszych funkcji C
    template<>
    struct CharTraits<char> {
        static size_t Length(const char* str) {
            return str ? std::strlen(str) : 0;
        }
        
        static int Compare(const char* s1, const char* s2, size_t n) {
            return std::memcmp(s1, s2, n);
        }
        
        static const char* Find(const char* str, size_t n, char c) {
            return static_cast<const char*>(std::memchr(str, c, n));
        }
        
        static char* Copy(char* dst, const char* src, size_t n) {
            return static_cast<char*>(std::memcpy(dst, src, n));
        }
        
        static char* Move(char* dst, const char* src, size_t n) {
            return static_cast<char*>(std::memmove(dst, src, n));
        }
        
        static char* Set(char* dst, char c, size_t n) {
            return static_cast<char*>(std::memset(dst, c, n));
        }
    };

    // Specjalizacja dla wchar_t - używa funkcji wide char
    template<>
    struct CharTraits<wchar_t> {
        static size_t Length(const wchar_t* str) {
            return str ? std::wcslen(str) : 0;
        }
        
        static int Compare(const wchar_t* s1, const wchar_t* s2, size_t n) {
            return std::wmemcmp(s1, s2, n);
        }
        
        static const wchar_t* Find(const wchar_t* str, size_t n, wchar_t c) {
            return std::wmemchr(str, c, n);
        }
        
        static wchar_t* Copy(wchar_t* dst, const wchar_t* src, size_t n) {
            return std::wmemcpy(dst, src, n);
        }
        
        static wchar_t* Move(wchar_t* dst, const wchar_t* src, size_t n) {
            return std::wmemmove(dst, src, n);
        }
        
        static wchar_t* Set(wchar_t* dst, wchar_t c, size_t n) {
            return std::wmemset(dst, c, n);
        }
    };

    template<typename CharT, typename Allocator = DefaultAllocator<CharT>>
    class BasicString {
    private:
        using Traits = CharTraits<CharT>;
        
        // SSO capacity zależy od rozmiaru CharT
        static constexpr size_t SSO_CAPACITY = (sizeof(CharT) == 1) ? 23 :
                                               (sizeof(CharT) == 2) ? 11 : 7;

        struct LongString {
            CharT* Data;
            size_t Size;
            size_t Capacity;
        };

        union {
            LongString m_Long;
            CharT m_Short[SSO_CAPACITY + 1];
        };

        [[no_unique_address]] Allocator m_Allocator;

        bool IsLong() const {
            return (m_Short[SSO_CAPACITY] & 0x80) != 0;
        }

        void SetLong(bool val) {
            if (val) {
                m_Short[SSO_CAPACITY] |= 0x80;
            } else {
                m_Short[SSO_CAPACITY] &= 0x7F;
            }
        }

        void SetShortSize(size_t sz) {
            assert(sz <= SSO_CAPACITY);
            m_Short[SSO_CAPACITY] = static_cast<CharT>((sz << 1) & 0x7F);
            // Upewnij się, że flaga "long" jest wyczyszczona
            m_Short[SSO_CAPACITY] &= 0x7F;
        }

        size_t GetShortSize() const {
            return static_cast<size_t>(m_Short[SSO_CAPACITY]) >> 1;
        }

        void GrowCapacity(size_t minCapacity) {
            size_t newCap = Capacity() * 2;
            if (newCap < minCapacity) {
                newCap = minCapacity;
            }
            Reserve(newCap);
        }

    public:
        using ValueType = CharT;
        using SizeType = size_t;
        using Iterator = CharT*;
        using ConstIterator = const CharT*;
        using AllocatorType = Allocator;

        static constexpr SizeType Npos = static_cast<SizeType>(-1);

        // Konstruktory
        explicit BasicString(const Allocator& allocator = Allocator()) : m_Allocator(allocator) {
            m_Short[0] = CharT(0);
            SetShortSize(0);
            SetLong(false);  // Jawnie ustaw tryb short
        }

        BasicString(const CharT* str, const Allocator& allocator = Allocator())
            : m_Allocator(allocator) {
            if (!str) {
                m_Short[0] = CharT(0);
                SetShortSize(0);
                return;
            }

            size_t len = Traits::Length(str);
            if (len <= SSO_CAPACITY) {
                Traits::Copy(m_Short, str, len);
                m_Short[len] = CharT(0);
                SetShortSize(len);
            } else {
                m_Long.Capacity = len + 1;
                m_Long.Size = len;
                m_Long.Data = m_Allocator.Allocate(m_Long.Capacity);
                Traits::Copy(m_Long.Data, str, len + 1);
                SetLong(true);
            }
        }

        BasicString(const CharT* str, size_t len, const Allocator& allocator = Allocator())
            : m_Allocator(allocator) {
            if (!str || len == 0) {
                m_Short[0] = CharT(0);
                SetShortSize(0);
                return;
            }

            if (len <= SSO_CAPACITY) {
                Traits::Copy(m_Short, str, len);
                m_Short[len] = CharT(0);
                SetShortSize(len);
            } else {
                m_Long.Capacity = len + 1;
                m_Long.Size = len;
                m_Long.Data = m_Allocator.Allocate(m_Long.Capacity);
                Traits::Copy(m_Long.Data, str, len);
                m_Long.Data[len] = CharT(0);
                SetLong(true);
            }
        }

        BasicString(size_t count, CharT c, const Allocator& allocator = Allocator())
            : m_Allocator(allocator) {
            if (count <= SSO_CAPACITY) {
                Traits::Set(m_Short, c, count);
                m_Short[count] = CharT(0);
                SetShortSize(count);
            } else {
                m_Long.Capacity = count + 1;
                m_Long.Size = count;
                m_Long.Data = m_Allocator.Allocate(m_Long.Capacity);
                Traits::Set(m_Long.Data, c, count);
                m_Long.Data[count] = CharT(0);
                SetLong(true);
            }
        }

        BasicString(const BasicString& other)
            : m_Allocator(other.m_Allocator) {
            if (other.IsLong()) {
                m_Long.Capacity = other.m_Long.Capacity;
                m_Long.Size = other.m_Long.Size;
                m_Long.Data = m_Allocator.Allocate(m_Long.Capacity);
                Traits::Copy(m_Long.Data, other.m_Long.Data, m_Long.Size + 1);
                SetLong(true);
            } else {
                Traits::Copy(m_Short, other.m_Short, SSO_CAPACITY + 1);
            }
        }

        BasicString(BasicString&& other) noexcept
    : m_Allocator(std::move(other.m_Allocator)) {
            if (other.IsLong()) {
                m_Long.Data = other.m_Long.Data;
                m_Long.Size = other.m_Long.Size;
                m_Long.Capacity = other.m_Long.Capacity;
                SetLong(true);

                // Zresetuj other do stanu short string
                other.m_Long.Data = nullptr;
                other.m_Long.Size = 0;
                other.m_Long.Capacity = 0;
                other.SetLong(false);
                other.m_Short[0] = CharT(0);
                other.SetShortSize(0);
            } else {
                Traits::Copy(m_Short, other.m_Short, SSO_CAPACITY + 1);
            }
        }

        ~BasicString() {
            if (IsLong()) {
                if (m_Long.Data && m_Long.Capacity > 0) {
                    m_Allocator.Deallocate(m_Long.Data, m_Long.Capacity);
                }
            }
        }

        // Operatory przypisania
        BasicString& operator=(const BasicString& other) {
            if (this != &other) {
                BasicString tmp(other);
                Swap(tmp);
            }
            return *this;
        }

        BasicString& operator=(BasicString&& other) noexcept {
            if (this != &other) {
                // Najpierw zwolnij obecną pamięć
                if (IsLong()) {
                    m_Allocator.Deallocate(m_Long.Data, m_Long.Capacity);
                }

                // Przenieś dane
                if (other.IsLong()) {
                    m_Long = other.m_Long;
                    SetLong(true);

                    // Zresetuj other
                    other.m_Long.Data = nullptr;
                    other.m_Long.Size = 0;
                    other.m_Long.Capacity = 0;
                    other.SetLong(false);
                } else {
                    Traits::Copy(m_Short, other.m_Short, SSO_CAPACITY + 1);
                }

                m_Allocator = std::move(other.m_Allocator);
            }
            return *this;
        }

        BasicString& operator=(const CharT* str) {
            BasicString tmp(str, m_Allocator);
            Swap(tmp);
            return *this;
        }

        // Dostęp do danych
        const CharT* CStr() const {
            return IsLong() ? m_Long.Data : m_Short;
        }

        CharT* Data() {
            return IsLong() ? m_Long.Data : m_Short;
        }

        const CharT* Data() const {
            return CStr();
        }

        size_t Size() const {
            return IsLong() ? m_Long.Size : GetShortSize();
        }

        size_t Length() const {
            return Size();
        }

        size_t Capacity() const {
            return IsLong() ? m_Long.Capacity - 1 : SSO_CAPACITY;
        }

        bool Empty() const {
            return Size() == 0;
        }

        // Operatory indeksowania
        CharT& operator[](size_t idx) {
            return Data()[idx];
        }

        const CharT& operator[](size_t idx) const {
            return Data()[idx];
        }

        CharT& At(size_t idx) {
            if (idx >= Size()) {
                throw std::out_of_range("BasicString::At");
            }
            return Data()[idx];
        }

        const CharT& At(size_t idx) const {
            if (idx >= Size()) {
                throw std::out_of_range("BasicString::At");
            }
            return Data()[idx];
        }

        CharT& Front() { return Data()[0]; }
        const CharT& Front() const { return Data()[0]; }
        CharT& Back() { return Data()[Size() - 1]; }
        const CharT& Back() const { return Data()[Size() - 1]; }

        // Iteratory
        Iterator Begin() { return Data(); }
        Iterator End() { return Data() + Size(); }
        ConstIterator Begin() const { return Data(); }
        ConstIterator End() const { return Data() + Size(); }
        ConstIterator CBegin() const { return Data(); }
        ConstIterator CEnd() const { return Data() + Size(); }

        // Reserve i Resize
        void Reserve(size_t newCap) {
            if (newCap <= Capacity()) return;

            size_t oldSize = Size();

            if (!IsLong() && newCap > SSO_CAPACITY) {
                CharT* newData = m_Allocator.Allocate(newCap + 1);
                Traits::Copy(newData, m_Short, oldSize + 1);

                m_Long.Data = newData;
                m_Long.Size = oldSize;
                m_Long.Capacity = newCap + 1;
                SetLong(true);
            } else if (IsLong()) {
                CharT* newData = m_Allocator.Allocate(newCap + 1);
                Traits::Copy(newData, m_Long.Data, oldSize + 1);
                m_Allocator.Deallocate(m_Long.Data, m_Long.Capacity);
                m_Long.Data = newData;
                m_Long.Capacity = newCap + 1;
            }
        }

        void Resize(size_t newSize, CharT c = CharT(0)) {
            size_t oldSize = Size();

            if (newSize > Capacity()) {
                Reserve(newSize);
            }

            if (newSize > oldSize) {
                Traits::Set(Data() + oldSize, c, newSize - oldSize);
            }

            Data()[newSize] = CharT(0);

            if (IsLong()) {
                m_Long.Size = newSize;
                // Jeśli rozmiar stał się mały, rozważ przejście do SSO
                if (newSize <= SSO_CAPACITY) {
                    ShrinkToFit();
                }
            } else if (newSize <= SSO_CAPACITY) {
                SetShortSize(newSize);
            } else {
                CharT temp[SSO_CAPACITY + 1];
                Traits::Copy(temp, m_Short, oldSize + 1);
                m_Long.Capacity = newSize + 1;
                m_Long.Size = newSize;
                m_Long.Data = m_Allocator.Allocate(m_Long.Capacity);
                Traits::Copy(m_Long.Data, temp, oldSize);
                Traits::Set(m_Long.Data + oldSize, c, newSize - oldSize);
                m_Long.Data[newSize] = CharT(0);
                SetLong(true);
            }
        }

        void ShrinkToFit() {
            if (!IsLong()) return;

            size_t sz = Size();
            if (sz <= SSO_CAPACITY) {
                CharT temp[SSO_CAPACITY + 1];
                Traits::Copy(temp, m_Long.Data, sz + 1);
                m_Allocator.Deallocate(m_Long.Data, m_Long.Capacity);
                Traits::Copy(m_Short, temp, sz + 1);
                SetShortSize(sz);
            } else if (m_Long.Capacity > sz + 1) {
                CharT* newData = m_Allocator.Allocate(sz + 1);
                Traits::Copy(newData, m_Long.Data, sz + 1);
                m_Allocator.Deallocate(m_Long.Data, m_Long.Capacity);
                m_Long.Data = newData;
                m_Long.Capacity = sz + 1;
            }
        }

        // Clear
        void Clear() {
            if (IsLong()) {
                m_Allocator.Deallocate(m_Long.Data, m_Long.Capacity);
                m_Long.Data = nullptr;
                m_Long.Size = 0;
                m_Long.Capacity = 0;
                SetLong(false);  // WAŻNE: przestaw na short mode!
            }
            m_Short[0] = CharT(0);
            SetShortSize(0);
        }

        // Append
        BasicString& Append(const CharT* str, size_t len) {
            if (!str || len == 0) return *this;

            size_t oldSize = Size();
            size_t newSize = oldSize + len;

            if (newSize > Capacity()) {
                GrowCapacity(newSize);
            }

            CharT* dst = Data() + oldSize;
            Traits::Copy(dst, str, len);
            dst[len] = CharT(0);

            if (IsLong()) {
                m_Long.Size = newSize;
            } else {
                SetShortSize(newSize);
            }

            return *this;
        }

        BasicString& Append(const CharT* str) {
            return str ? Append(str, Traits::Length(str)) : *this;
        }

        BasicString& Append(const BasicString& str) {
            return Append(str.Data(), str.Size());
        }

        BasicString& Append(CharT c) {
            return Append(&c, 1);
        }

        BasicString& operator+=(const CharT* str) {
            return Append(str);
        }

        BasicString& operator+=(const BasicString& str) {
            return Append(str);
        }

        BasicString& operator+=(CharT c) {
            return Append(c);
        }

        // Insert
        BasicString& Insert(size_t pos, const CharT* str, size_t len) {
            if (!str || len == 0) return *this;
            if (pos > Size()) throw std::out_of_range("BasicString::Insert");

            size_t oldSize = Size();
            size_t newSize = oldSize + len;

            if (newSize > Capacity()) {
                GrowCapacity(newSize);
            }

            CharT* data = Data();
            Traits::Move(data + pos + len, data + pos, oldSize - pos);
            Traits::Copy(data + pos, str, len);
            data[newSize] = CharT(0);

            if (IsLong()) {
                m_Long.Size = newSize;
            } else {
                SetShortSize(newSize);
            }

            return *this;
        }

        BasicString& Insert(size_t pos, const CharT* str) {
            return str ? Insert(pos, str, Traits::Length(str)) : *this;
        }

        BasicString& Insert(size_t pos, const BasicString& str) {
            return Insert(pos, str.Data(), str.Size());
        }

        // Erase
        BasicString& Erase(size_t pos = 0, size_t count = Npos) {
            size_t sz = Size();
            if (pos > sz) throw std::out_of_range("BasicString::Erase");

            size_t len = std::min<size_t>(count, sz - pos);
            if (len == 0) return *this;

            CharT* data = Data();
            Traits::Move(data + pos, data + pos + len, sz - pos - len + 1);

            size_t newSize = sz - len;
            if (IsLong()) {
                m_Long.Size = newSize;
            } else {
                SetShortSize(newSize);
            }

            return *this;
        }

        // Replace
        BasicString& Replace(size_t pos, size_t count, const CharT* str, size_t strLen) {
            if (!str) return Erase(pos, count);
            if (pos > Size()) throw std::out_of_range("BasicString::Replace");

            size_t sz = Size();
            count = std::min<size_t>(count, sz - pos);

            if (strLen == count) {
                Traits::Copy(Data() + pos, str, strLen);
            } else if (strLen < count) {
                Traits::Copy(Data() + pos, str, strLen);
                Erase(pos + strLen, count - strLen);
            } else {
                Erase(pos, count);
                Insert(pos, str, strLen);
            }

            return *this;
        }

        BasicString& Replace(size_t pos, size_t count, const CharT* str) {
            return str ? Replace(pos, count, str, Traits::Length(str)) : Erase(pos, count);
        }

        BasicString& Replace(size_t pos, size_t count, const BasicString& str) {
            return Replace(pos, count, str.Data(), str.Size());
        }

        // Find
        SizeType Find(const CharT* str, SizeType pos = 0) const {
            if (!str || pos >= Size()) return Npos;

            size_t strLen = Traits::Length(str);
            if (strLen == 0) return pos;
            if (strLen > Size() - pos) return Npos;

            const CharT* data = Data();
            for (size_t i = pos; i <= Size() - strLen; ++i) {
                if (Traits::Compare(data + i, str, strLen) == 0) {
                    return i;
                }
            }

            return Npos;
        }

        SizeType Find(CharT c, SizeType pos = 0) const {
            if (pos >= Size()) return Npos;

            const CharT* result = Traits::Find(Data() + pos, Size() - pos, c);

            return result ? static_cast<SizeType>(result - Data()) : Npos;
        }

        SizeType Find(const BasicString& str, SizeType pos = 0) const {
            return Find(str.Data(), pos);
        }

        // RFind (szukanie od końca)
        SizeType RFind(CharT c, SizeType pos = Npos) const {
            if (Empty()) return Npos;

            size_t sz = Size();
            size_t start = (pos == Npos || pos >= sz) ? sz - 1 : pos;

            const CharT* data = Data();
            for (size_t i = start + 1; i > 0; --i) {
                if (data[i - 1] == c) {
                    return i - 1;
                }
            }

            return Npos;
        }

        SizeType RFind(const CharT* str, SizeType pos = Npos) const {
            if (!str) return Npos;

            size_t strLen = Traits::Length(str);
            if (strLen == 0) return (pos == Npos || pos >= Size()) ? Size() : pos;

            size_t sz = Size();
            if (strLen > sz) return Npos;

            size_t start = (pos == Npos || pos > sz - strLen) ? sz - strLen : pos;
            const CharT* data = Data();

            for (size_t i = start + 1; i > 0; --i) {
                if (Traits::Compare(data + i - 1, str, strLen) == 0) {
                    return i - 1;
                }
            }

            return Npos;
        }

        bool Contains(const char* str) const
        {
            return Find(str) != -1;
        }

        bool Contains(const BasicString& str) const
        {
            return Find(str) != -1;
        }

        bool Contains(char c) const
        {
            return Find(c) != -1;
        }

        //Split
        std::vector<BasicString> Split(char delimiter, bool skipEmpty = false) const {
            std::vector<BasicString> result;
            size_t start = 0;
            size_t end = Find(delimiter);

            while (end != Npos) {
                if (!skipEmpty || end > start) {
                    result.push_back(Substr(start, end - start));
                }
                start = end + 1;
                end = Find(delimiter, start);
            }

            if (!skipEmpty || start < Size()) {
                result.push_back(Substr(start));
            }

            return result;
        }

        std::vector<BasicString> Split(const char* delimiter, bool skipEmpty = false) const {
            std::vector<BasicString> result;
            if (!delimiter || std::strlen(delimiter) == 0) {
                result.push_back(*this);
                return result;
            }

            size_t start = 0;
            size_t delimLen = std::strlen(delimiter);
            size_t end = Find(delimiter);

            while (end != Npos) {
                if (!skipEmpty || end > start) {
                    result.push_back(Substr(start, end - start));
                }
                start = end + delimLen;
                end = Find(delimiter, start);
            }

            if (!skipEmpty || start < Size()) {
                result.push_back(Substr(start));
            }

            return result;
        }

        //Join
        static BasicString Join(const std::vector<BasicString>& elements, const char* delimiter = "", const Allocator& allocator = Allocator()) {
            if (elements.empty()) {
                return BasicString(allocator);
            }

            size_t delimLen = delimiter ? std::strlen(delimiter) : 0;
            size_t totalSize = 0;

            for (size_t i = 0; i < elements.size(); ++i) {
                totalSize += elements[i].Size();
                if (i < elements.size() - 1) {
                    totalSize += delimLen;
                }
            }

            BasicString result(allocator);
            result.Reserve(totalSize);

            for (size_t i = 0; i < elements.size(); ++i) {
                result.Append(elements[i]);
                if (i < elements.size() - 1 && delimiter) {
                    result.Append(delimiter);
                }
            }

            return result;
        }

        // FindFirstOf / FindLastOf
        SizeType FindFirstOf(const CharT* chars, SizeType pos = 0) const {
            if (!chars || pos >= Size()) return Npos;

            const CharT* data = Data();
            size_t sz = Size();
            size_t charsLen = Traits::Length(chars);

            for (size_t i = pos; i < sz; ++i) {
                for (size_t j = 0; j < charsLen; ++j) {
                    if (data[i] == chars[j]) {
                        return i;
                    }
                }
            }

            return Npos;
        }

        SizeType FindLastOf(const CharT* chars, SizeType pos = Npos) const {
            if (!chars || Empty()) return Npos;

            size_t sz = Size();
            size_t start = (pos == Npos || pos >= sz) ? sz - 1 : pos;
            const CharT* data = Data();
            size_t charsLen = Traits::Length(chars);

            for (size_t i = start + 1; i > 0; --i) {
                for (size_t j = 0; j < charsLen; ++j) {
                    if (data[i - 1] == chars[j]) {
                        return i - 1;
                    }
                }
            }

            return Npos;
        }

        // Substr
        BasicString Substr(SizeType pos = 0, SizeType count = Npos) const {
            if (pos > Size()) throw std::out_of_range("BasicString::Substr");

            SizeType len = std::min<size_t>(count, Size() - pos);
            return BasicString(Data() + pos, len, m_Allocator);
        }

        // Porównania
        int Compare(const BasicString& other) const {
            size_t len = std::min<size_t>(Size(), other.Size());
            int result = Traits::Compare(Data(), other.Data(), len);
            if (result != 0) return result;
            if (Size() < other.Size()) return -1;
            if (Size() > other.Size()) return 1;
            return 0;
        }

        int Compare(const CharT* str) const {
            if (!str) return Empty() ? 0 : 1;

            size_t strLen = Traits::Length(str);
            size_t len = std::min<size_t>(Size(), strLen);
            int result = Traits::Compare(Data(), str, len);
            if (result != 0) return result;
            if (Size() < strLen) return -1;
            if (Size() > strLen) return 1;
            return 0;
        }

        bool operator==(const BasicString& other) const {
            return Size() == other.Size() &&
                   Traits::Compare(Data(), other.Data(), Size()) == 0;
        }

        bool operator!=(const BasicString& other) const {
            return !(*this == other);
        }

        bool operator<(const BasicString& other) const {
            return Compare(other) < 0;
        }

        bool operator<=(const BasicString& other) const {
            return Compare(other) <= 0;
        }

        bool operator>(const BasicString& other) const {
            return Compare(other) > 0;
        }

        bool operator>=(const BasicString& other) const {
            return Compare(other) >= 0;
        }

        bool operator==(const CharT* str) const {
            if (!str) return Empty();
            size_t len = Traits::Length(str);
            return Size() == len && Traits::Compare(Data(), str, len) == 0;
        }

        bool operator!=(const CharT* str) const {
            return !(*this == str);
        }

        // StartsWith / EndsWith
        bool StartsWith(const CharT* prefix) const {
            if (!prefix) return true;
            size_t prefixLen = Traits::Length(prefix);
            return Size() >= prefixLen &&
                   Traits::Compare(Data(), prefix, prefixLen) == 0;
        }

        bool StartsWith(CharT c) const {
            return !Empty() && Front() == c;
        }

        bool StartsWith(const BasicString& prefix) const {
            return Size() >= prefix.Size() &&
                   Traits::Compare(Data(), prefix.Data(), prefix.Size()) == 0;
        }

        bool EndsWith(const CharT* suffix) const {
            if (!suffix) return true;
            size_t suffixLen = Traits::Length(suffix);
            return Size() >= suffixLen &&
                   Traits::Compare(Data() + Size() - suffixLen, suffix, suffixLen) == 0;
        }

        bool EndsWith(CharT c) const {
            return !Empty() && Back() == c;
        }

        bool EndsWith(const BasicString& suffix) const {
            return Size() >= suffix.Size() &&
                   Traits::Compare(Data() + Size() - suffix.Size(),
                              suffix.Data(), suffix.Size()) == 0;
        }

        // ToLower / ToUpper (tylko dla char)
        BasicString ToLower() const {
            BasicString result(*this);
            CharT* data = result.Data();
            for (size_t i = 0; i < result.Size(); ++i) {
                if (sizeof(CharT) == sizeof(char)) {
                    data[i] = static_cast<CharT>(std::tolower(static_cast<unsigned char>(data[i])));
                }
            }
            return result;
        }

        BasicString ToUpper() const {
            BasicString result(*this);
            CharT* data = result.Data();
            for (size_t i = 0; i < result.Size(); ++i) {
                if (sizeof(CharT) == sizeof(char)) {
                    data[i] = static_cast<CharT>(std::toupper(static_cast<unsigned char>(data[i])));
                }
            }
            return result;
        }

        void ToLowerInPlace() {
            CharT* data = Data();
            for (size_t i = 0; i < Size(); ++i) {
                if (sizeof(CharT) == sizeof(char)) {
                    data[i] = static_cast<CharT>(std::tolower(static_cast<unsigned char>(data[i])));
                }
            }
        }

        void ToUpperInPlace() {
            CharT* data = Data();
            for (size_t i = 0; i < Size(); ++i) {
                if (sizeof(CharT) == sizeof(char)) {
                    data[i] = static_cast<CharT>(std::toupper(static_cast<unsigned char>(data[i])));
                }
            }
        }

        // Trim (tylko dla char)
        BasicString TrimLeft() const {
            const CharT* data = Data();
            size_t sz = Size();
            size_t start = 0;

            if (sizeof(CharT) == sizeof(char)) {
                while (start < sz && std::isspace(static_cast<unsigned char>(data[start]))) {
                    ++start;
                }
            }

            return BasicString(data + start, sz - start, m_Allocator);
        }

        BasicString TrimRight() const {
            const CharT* data = Data();
            size_t sz = Size();
            size_t end = sz;

            if (sizeof(CharT) == sizeof(char)) {
                while (end > 0 && std::isspace(static_cast<unsigned char>(data[end - 1]))) {
                    --end;
                }
            }

            return BasicString(data, end, m_Allocator);
        }

        BasicString Trim() const {
            const CharT* data = Data();
            size_t sz = Size();
            size_t start = 0;
            size_t end = sz;

            if (sizeof(CharT) == sizeof(char)) {
                while (start < sz && std::isspace(static_cast<unsigned char>(data[start]))) {
                    ++start;
                }

                while (end > start && std::isspace(static_cast<unsigned char>(data[end - 1]))) {
                    --end;
                }
            }

            return BasicString(data + start, end - start, m_Allocator);
        }

        // Hash (FNV-1a)
        size_t Hash() const {
            const size_t FNV_PRIME = 0x100000001b3;
            const size_t FNV_OFFSET = 0xcbf29ce484222325;

            size_t hash = FNV_OFFSET;
            const CharT* data = Data();

            for (size_t i = 0; i < Size(); ++i) {
                hash ^= static_cast<size_t>(data[i]);
                hash *= FNV_PRIME;
            }

            return hash;
        }

        // Konwersje numeryczne (tylko dla char)
        int ToInt(int defaultValue = 0) const {
            if (Empty() || sizeof(CharT) != sizeof(char)) return defaultValue;

            char* end;
            long result = std::strtol(reinterpret_cast<const char*>(Data()), &end, 10);

            return (end != reinterpret_cast<const char*>(Data())) ? static_cast<int>(result) : defaultValue;
        }

        float ToFloat(float defaultValue = 0.0f) const {
            if (Empty() || sizeof(CharT) != sizeof(char)) return defaultValue;

            char* end;
            float result = std::strtof(reinterpret_cast<const char*>(Data()), &end);

            return (end != reinterpret_cast<const char*>(Data())) ? result : defaultValue;
        }

        double ToDouble(double defaultValue = 0.0) const {
            if (Empty() || sizeof(CharT) != sizeof(char)) return defaultValue;

            char* end;
            double result = std::strtod(reinterpret_cast<const char*>(Data()), &end);

            return (end != reinterpret_cast<const char*>(Data())) ? result : defaultValue;
        }

        static BasicString FromInt(int value, const Allocator& allocator = Allocator()) {
            if (sizeof(CharT) != sizeof(char)) return BasicString(allocator);

            char buffer[32];
            int len = std::snprintf(buffer, sizeof(buffer), "%d", value);
            return BasicString(reinterpret_cast<const CharT*>(buffer), len, allocator);
        }

        static BasicString FromFloat(float value, int precision = 6, const Allocator& allocator = Allocator()) {
            if (sizeof(CharT) != sizeof(char)) return BasicString(allocator);

            char buffer[64];
            int len = std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
            return BasicString(reinterpret_cast<const CharT*>(buffer), len, allocator);
        }

        static BasicString FromDouble(double value, int precision = 6, const Allocator& allocator = Allocator()) {
            if (sizeof(CharT) != sizeof(char)) return BasicString(allocator);

            char buffer[64];
            int len = std::snprintf(buffer, sizeof(buffer), "%.*lf", precision, value);
            return BasicString(reinterpret_cast<const CharT*>(buffer), len, allocator);
        }

        // Format (tylko dla char)
        template<typename... Args>
        static BasicString Format(const CharT* fmt, Args... args) {
            return Format(Allocator(), fmt, args...);
        }

        template<typename... Args>
        static BasicString Format(const Allocator& allocator, const CharT* fmt, Args... args) {
            if (sizeof(CharT) != sizeof(char) || !fmt) return BasicString(allocator);

            char buffer[1024];
            int len = std::snprintf(buffer, sizeof(buffer), reinterpret_cast<const char*>(fmt), args...);

            if (len < 0) return BasicString(allocator);
            if (len < static_cast<int>(sizeof(buffer))) {
                return BasicString(reinterpret_cast<const CharT*>(buffer), len, allocator);
            }

            BasicString result(allocator);
            result.Resize(len);
            std::snprintf(reinterpret_cast<char*>(result.Data()), len + 1,
                         reinterpret_cast<const char*>(fmt), args...);
            return result;
        }

        // Path utilities (działa dla char i wchar_t)
        BasicString GetFileName() const {
            SizeType pos = RFind(CharT('/'));
            if (pos == Npos) {
                pos = RFind(CharT('\\'));
            }

            return (pos == Npos) ? *this : Substr(pos + 1);
        }

        BasicString GetFileExtension() const {
            SizeType pos = RFind(CharT('.'));
            SizeType slash = RFind(CharT('/'));
            SizeType backslash = RFind(CharT('\\'));

            SizeType lastSlash = (slash == Npos) ? backslash :
                                (backslash == Npos) ? slash :
                                std::max<size_t>(slash, backslash);

            if (pos == Npos || (lastSlash != Npos && pos < lastSlash)) {
                return BasicString(m_Allocator);
            }

            return Substr(pos);
        }

        BasicString GetDirectory() const {
            SizeType pos = RFind(CharT('/'));
            if (pos == Npos) {
                pos = RFind(CharT('\\'));
            }

            return (pos == Npos) ? BasicString(m_Allocator) : Substr(0, pos);
        }

        BasicString RemoveExtension() const {
            SizeType pos = RFind(CharT('.'));
            SizeType slash = RFind(CharT('/'));
            SizeType backslash = RFind(CharT('\\'));

            SizeType lastSlash = (slash == Npos) ? backslash :
                                (backslash == Npos) ? slash :
                                std::max<size_t>(slash, backslash);

            if (pos == Npos || (lastSlash != Npos && pos < lastSlash)) {
                return *this;
            }

            return Substr(0, pos);
        }

        // Swap
        void Swap(BasicString& other) noexcept {
            // NIE UŻYWAJ memcpy!
            // Przenieś zawartość bezpiecznie:
            BasicString temp(std::move(*this));
            *this = std::move(other);
            other = std::move(temp);
        }

        // Konwersja z wchar_t na char (tylko gdy CharT to char)
        static BasicString<char, Allocator> FromWide(const wchar_t* wstr, const Allocator& allocator = Allocator()) {
            if (!wstr) return BasicString<char, Allocator>(allocator);

            // Pobierz wymaganą długość (bez terminatora)
            size_t len = std::wcstombs(nullptr, wstr, 0);
            if (len == static_cast<size_t>(-1)) return BasicString<char, Allocator>(allocator);

            BasicString<char, Allocator> result(allocator);
            result.Resize(len);

            // Wykonaj właściwą konwersję
            std::wcstombs(reinterpret_cast<char*>(result.Data()), wstr, len + 1);
            return result;
        }

        // Konwersja z char na wchar_t (tylko gdy CharT to wchar_t)
        static BasicString<wchar_t, Allocator> FromNarrow(const char* str, const Allocator& allocator = Allocator()) {
            if (!str) return BasicString<wchar_t, Allocator>(allocator);

            // Pobierz wymaganą długość
            size_t len = std::mbstowcs(nullptr, str, 0);
            if (len == static_cast<size_t>(-1)) return BasicString<wchar_t, Allocator>(allocator);

            BasicString<wchar_t, Allocator> result(allocator);
            result.Resize(len);

            // Wykonaj właściwą konwersję
            std::mbstowcs(result.Data(), str, len + 1);
            return result;
        }
    };

    // Operatory concatenacji
    template<typename CharT, typename A>
    BasicString<CharT, A> operator+(const BasicString<CharT, A>& lhs, const BasicString<CharT, A>& rhs) {
        BasicString<CharT, A> result;
        result.Reserve(lhs.Size() + rhs.Size());
        result.Append(lhs);
        result.Append(rhs);
        return result;
    }

    template<typename CharT, typename A>
    BasicString<CharT, A> operator+(const BasicString<CharT, A>& lhs, const CharT* rhs) {
        BasicString<CharT, A> result;
        size_t rhsLen = rhs ? CharTraits<CharT>::Length(rhs) : 0;
        result.Reserve(lhs.Size() + rhsLen);
        result.Append(lhs);
        result.Append(rhs);
        return result;
    }

    template<typename CharT, typename A>
    BasicString<CharT, A> operator+(const CharT* lhs, const BasicString<CharT, A>& rhs) {
        BasicString<CharT, A> result;
        size_t lhsLen = lhs ? CharTraits<CharT>::Length(lhs) : 0;
        result.Reserve(lhsLen + rhs.Size());
        result.Append(lhs);
        result.Append(rhs);
        return result;
    }

    // Stream output
    template<typename CharT, typename A>
    std::ostream& operator<<(std::ostream& os, const BasicString<CharT, A>& str) {
        if (sizeof(CharT) == sizeof(char)) {
            return os.write(reinterpret_cast<const char*>(str.Data()), str.Size());
        }
        return os;
    }

    template<typename A>
    std::wostream& operator<<(std::wostream& os, const BasicString<wchar_t, A>& str) {
        return os.write(str.Data(), str.Size());
    }

    // Aliasy dla wygody
    using String = BasicString<char, DefaultAllocator<char>>;
    using StringW = BasicString<wchar_t, DefaultAllocator<wchar_t>>;
}

// Hash specialization dla std::unordered_map
namespace std {
    template<typename CharT, typename A>
    struct hash<Plu::BasicString<CharT, A>> {
        size_t operator()(const Plu::BasicString<CharT, A>& str) const {
            return str.Hash();
        }
    };
}