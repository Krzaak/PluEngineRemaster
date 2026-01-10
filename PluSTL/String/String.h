#pragma once

#include <cstring>
#include <cwchar>
#include <cctype>
#include <cwctype>
#include <cstddef>
#include <utility>
#include <type_traits>
#include "Allocators/Default.h"

namespace Plu
{
    // =============================================================================
    // BasicString CLASS
    // =============================================================================

    template<typename CharT, typename Allocator = DefaultAllocator<CharT>>
    class BasicString {
    public:
        using ValueType = CharT;
        using SizeType = std::size_t;
        static constexpr SizeType Npos = static_cast<SizeType>(-1);
        
        // Small BasicString Optimization - 23 chars for char, 11 for wchar_t (typical)
        static constexpr SizeType SsoCapacity = (24 / sizeof(CharT)) - 1;
        
    private:
        // SSO: Store short strings inline to avoid heap allocation
        union {
            CharT mSsoBuffer[SsoCapacity + 1];
            CharT* mHeapData;
        };
        
        SizeType mLength;
        SizeType mCapacity;
        Allocator mAllocator;
        
        // Flag to determine if using heap (stored in LSB of capacity for space efficiency)
        [[nodiscard]] bool IsHeap() const noexcept {
            return mCapacity > SsoCapacity;
        }
        
        [[nodiscard]] CharT* GetData() noexcept {
            return IsHeap() ? mHeapData : mSsoBuffer;
        }
        
        [[nodiscard]] const CharT* GetData() const noexcept {
            return IsHeap() ? mHeapData : mSsoBuffer;
        }
        
        // BasicString utility functions - conditionally compiled based on CharT
        static SizeType StrLen(const CharT* str) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return std::strlen(str);
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                return std::wcslen(str);
            }
        }
        
        static void StrCopy(CharT* dest, const CharT* src, SizeType count) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                std::memcpy(dest, src, count * sizeof(CharT));
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                std::wmemcpy(dest, src, count);
            }
        }
        
        static int StrCompare(const CharT* s1, const CharT* s2, SizeType count) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return std::memcmp(s1, s2, count * sizeof(CharT));
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                return std::wmemcmp(s1, s2, count);
            }
        }
        
        static CharT ToLowerChar(CharT c) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return static_cast<CharT>(std::tolower(static_cast<unsigned char>(c)));
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                return static_cast<CharT>(std::towlower(static_cast<std::wint_t>(c)));
            }
        }
        
        static CharT ToUpperChar(CharT c) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return static_cast<CharT>(std::toupper(static_cast<unsigned char>(c)));
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                return static_cast<CharT>(std::towupper(static_cast<std::wint_t>(c)));
            }
        }
        
        void AllocateHeap(SizeType capacity) noexcept {
            mHeapData = mAllocator.Allocate(capacity + 1);
            mCapacity = capacity;
        }
        
        void DeallocateHeap() noexcept {
            if (IsHeap() && mHeapData) {
                mAllocator.Deallocate(mHeapData, mCapacity + 1);
            }
        }
        
        // Grow capacity using 1.5x growth factor (common in game engines)
        SizeType CalculateGrowth(SizeType newLength) const noexcept {
            SizeType newCapacity = mCapacity + mCapacity / 2;
            return (newCapacity < newLength) ? newLength : newCapacity;
        }

    public:
        // =========================================================================
        // CONSTRUCTORS & DESTRUCTOR
        // =========================================================================
        
        BasicString() noexcept : mLength(0), mCapacity(SsoCapacity) {
            mSsoBuffer[0] = CharT{0};
        }
        
        BasicString(const CharT* str) noexcept : BasicString() {
            if (!str) return;
            
            SizeType len = StrLen(str);
            if (len <= SsoCapacity) {
                StrCopy(mSsoBuffer, str, len);
                mSsoBuffer[len] = CharT{0};
                mLength = len;
            } else {
                AllocateHeap(len);
                if (mHeapData) {
                    StrCopy(mHeapData, str, len);
                    mHeapData[len] = CharT{0};
                    mLength = len;
                }
            }
        }
        
        BasicString(const CharT* str, SizeType length) noexcept : BasicString() {
            if (!str || length == 0) return;
            
            if (length <= SsoCapacity) {
                StrCopy(mSsoBuffer, str, length);
                mSsoBuffer[length] = CharT{0};
                mLength = length;
            } else {
                AllocateHeap(length);
                if (mHeapData) {
                    StrCopy(mHeapData, str, length);
                    mHeapData[length] = CharT{0};
                    mLength = length;
                }
            }
        }
        
        // Copy constructor
        BasicString(const BasicString& other) noexcept : mLength(other.mLength), mCapacity(other.mCapacity) {
            if (other.IsHeap()) {
                AllocateHeap(other.mCapacity);
                if (mHeapData) {
                    StrCopy(mHeapData, other.mHeapData, mLength + 1);
                }
            } else {
                StrCopy(mSsoBuffer, other.mSsoBuffer, mLength + 1);
            }
        }
        
        // Move constructor
        BasicString(BasicString&& other) noexcept : mLength(other.mLength), mCapacity(other.mCapacity) {
            if (other.IsHeap()) {
                mHeapData = other.mHeapData;
                other.mHeapData = nullptr;
                other.mLength = 0;
                other.mCapacity = SsoCapacity;
            } else {
                StrCopy(mSsoBuffer, other.mSsoBuffer, mLength + 1);
            }
        }
        
        ~BasicString() noexcept {
            DeallocateHeap();
        }
        
        // =========================================================================
        // ASSIGNMENT OPERATORS
        // =========================================================================
        
        BasicString& operator=(const BasicString& other) noexcept {
            if (this != &other) {
                DeallocateHeap();
                
                mLength = other.mLength;
                mCapacity = other.mCapacity;
                
                if (other.IsHeap()) {
                    AllocateHeap(other.mCapacity);
                    if (mHeapData) {
                        StrCopy(mHeapData, other.mHeapData, mLength + 1);
                    }
                } else {
                    StrCopy(mSsoBuffer, other.mSsoBuffer, mLength + 1);
                }
            }
            return *this;
        }
        
        BasicString& operator=(BasicString&& other) noexcept {
            if (this != &other) {
                DeallocateHeap();
                
                mLength = other.mLength;
                mCapacity = other.mCapacity;
                
                if (other.IsHeap()) {
                    mHeapData = other.mHeapData;
                    other.mHeapData = nullptr;
                    other.mLength = 0;
                    other.mCapacity = SsoCapacity;
                } else {
                    StrCopy(mSsoBuffer, other.mSsoBuffer, mLength + 1);
                }
            }
            return *this;
        }
        
        // =========================================================================
        // BASIC OPERATIONS
        // =========================================================================
        
        [[nodiscard]] SizeType Length() const noexcept { return mLength; }
        [[nodiscard]] SizeType Capacity() const noexcept { return mCapacity; }
        [[nodiscard]] bool IsEmpty() const noexcept { return mLength == 0; }
        [[nodiscard]] const CharT* CStr() const noexcept { return GetData(); }
        
        void Clear() noexcept {
            mLength = 0;
            GetData()[0] = CharT{0};
        }
        
        [[nodiscard]] CharT& operator[](SizeType index) noexcept {
            return GetData()[index];
        }
        
        [[nodiscard]] const CharT& operator[](SizeType index) const noexcept {
            return GetData()[index];
        }
        
        void Reserve(SizeType newCapacity) noexcept {
            if (newCapacity <= mCapacity) return;
            
            CharT* newData = mAllocator.Allocate(newCapacity + 1);
            if (!newData) return;
            
            StrCopy(newData, GetData(), mLength + 1);
            DeallocateHeap();
            
            mHeapData = newData;
            mCapacity = newCapacity;
        }
        
        // =========================================================================
        // SEARCH OPERATIONS
        // =========================================================================
        
        [[nodiscard]] SizeType Find(CharT c, SizeType startPos = 0) const noexcept {
            if (startPos >= mLength) return Npos;
            
            const CharT* data = GetData();
            for (SizeType i = startPos; i < mLength; ++i) {
                if (data[i] == c) return i;
            }
            return Npos;
        }
        
        [[nodiscard]] SizeType Find(const CharT* substr, SizeType startPos = 0) const noexcept {
            if (!substr || startPos >= mLength) return Npos;
            
            SizeType subLen = StrLen(substr);
            if (subLen == 0 || subLen > mLength - startPos) return Npos;
            
            const CharT* data = GetData();
            for (SizeType i = startPos; i <= mLength - subLen; ++i) {
                if (StrCompare(data + i, substr, subLen) == 0) {
                    return i;
                }
            }
            return Npos;
        }
        
        [[nodiscard]] SizeType RFind(CharT c, SizeType startPos = Npos) const noexcept {
            if (mLength == 0) return Npos;
            
            SizeType pos = (startPos == Npos || startPos >= mLength) ? mLength - 1 : startPos;
            const CharT* data = GetData();
            
            for (SizeType i = pos + 1; i > 0; --i) {
                if (data[i - 1] == c) return i - 1;
            }
            return Npos;
        }
        
        [[nodiscard]] bool Contains(const CharT* substr) const noexcept {
            return Find(substr) != Npos;
        }
        
        [[nodiscard]] bool StartsWith(const CharT* prefix) const noexcept {
            if (!prefix) return false;
            SizeType prefixLen = StrLen(prefix);
            if (prefixLen > mLength) return false;
            return StrCompare(GetData(), prefix, prefixLen) == 0;
        }
        
        [[nodiscard]] bool EndsWith(const CharT* suffix) const noexcept {
            if (!suffix) return false;
            SizeType suffixLen = StrLen(suffix);
            if (suffixLen > mLength) return false;
            return StrCompare(GetData() + mLength - suffixLen, suffix, suffixLen) == 0;
        }
        
        // =========================================================================
        // SUBSTRING & MODIFICATION
        // =========================================================================
        
        [[nodiscard]] BasicString Substring(SizeType start, SizeType length = Npos) const noexcept {
            if (start >= mLength) return BasicString();
            
            SizeType actualLength = (length == Npos || start + length > mLength) 
                                    ? mLength - start : length;
            return BasicString(GetData() + start, actualLength);
        }
        
        void Append(const CharT* str) noexcept {
            if (!str) return;
            
            SizeType addLen = StrLen(str);
            if (addLen == 0) return;
            
            SizeType newLength = mLength + addLen;
            if (newLength > mCapacity) {
                Reserve(CalculateGrowth(newLength));
            }
            
            StrCopy(GetData() + mLength, str, addLen);
            mLength = newLength;
            GetData()[mLength] = CharT{0};
        }
        
        void Append(const BasicString& other) noexcept {
            Append(other.CStr());
        }
        
        void Insert(SizeType pos, const CharT* str) noexcept {
            if (!str || pos > mLength) return;
            
            SizeType insertLen = StrLen(str);
            if (insertLen == 0) return;
            
            SizeType newLength = mLength + insertLen;
            if (newLength > mCapacity) {
                Reserve(CalculateGrowth(newLength));
            }
            
            CharT* data = GetData();
            // Move existing content
            for (SizeType i = mLength; i >= pos && i > 0; --i) {
                data[i + insertLen] = data[i];
            }
            
            // Insert new content
            StrCopy(data + pos, str, insertLen);
            mLength = newLength;
            data[mLength] = CharT{0};
        }
        
        void Remove(SizeType start, SizeType length = Npos) noexcept {
            if (start >= mLength) return;
            
            SizeType actualLength = (length == Npos || start + length > mLength) 
                                    ? mLength - start : length;
            
            CharT* data = GetData();
            SizeType remaining = mLength - start - actualLength;
            
            if (remaining > 0) {
                StrCopy(data + start, data + start + actualLength, remaining);
            }
            
            mLength -= actualLength;
            data[mLength] = CharT{0};
        }
        
        void Replace(const CharT* oldStr, const CharT* newStr) noexcept {
            if (!oldStr || !newStr) return;
            
            SizeType oldLen = StrLen(oldStr);
            SizeType newLen = StrLen(newStr);
            
            SizeType pos = Find(oldStr);
            if (pos == Npos) return;
            
            Remove(pos, oldLen);
            Insert(pos, newStr);
        }
        
        // =========================================================================
        // CASE OPERATIONS
        // =========================================================================
        
        void ToLowerInPlace() noexcept {
            CharT* data = GetData();
            for (SizeType i = 0; i < mLength; ++i) {
                data[i] = ToLowerChar(data[i]);
            }
        }
        
        void ToUpperInPlace() noexcept {
            CharT* data = GetData();
            for (SizeType i = 0; i < mLength; ++i) {
                data[i] = ToUpperChar(data[i]);
            }
        }
        
        [[nodiscard]] BasicString ToLower() const noexcept {
            BasicString result(*this);
            result.ToLowerInPlace();
            return result;
        }
        
        [[nodiscard]] BasicString ToUpper() const noexcept {
            BasicString result(*this);
            result.ToUpperInPlace();
            return result;
        }
        
        // =========================================================================
        // NUMERIC CONVERSION TO BasicString
        // =========================================================================
        
        // Convert integer to BasicString
        template<typename IntT>
        static BasicString FromInt(IntT value) noexcept {
            static_assert(std::is_integral_v<IntT>, "FromInt requires integral type");
            
            // Buffer size: max digits for 64-bit int (20) + sign + null
            CharT buffer[32];
            CharT* ptr = buffer + 31;
            *ptr = CharT{0};
            
            bool negative = false;
            if constexpr (std::is_signed_v<IntT>) {
                if (value < 0) {
                    negative = true;
                    value = -value;
                }
            }
            
            // Convert digits in reverse
            using UnsignedT = typename std::make_unsigned<IntT>::type;
            UnsignedT uvalue = static_cast<UnsignedT>(value);
            
            if (uvalue == 0) {
                *(--ptr) = GetDigitChar(0);
            } else {
                while (uvalue > 0) {
                    *(--ptr) = GetDigitChar(uvalue % 10);
                    uvalue /= 10;
                }
            }
            
            if (negative) {
                *(--ptr) = GetMinusChar();
            }
            
            return BasicString(ptr);
        }
        
        // Convert float/double to BasicString with specified precision
        template<typename FloatT>
        static BasicString FromFloat(FloatT value, int precision = 6) noexcept {
            static_assert(std::is_floating_point_v<FloatT>, "FromFloat requires floating point type");
            
            CharT buffer[64];
            
            // Handle special cases
            if (value != value) { // NaN check
                return BasicString(GetNaNStr());
            }
            if (value == std::numeric_limits<FloatT>::infinity()) {
                return BasicString(GetInfStr());
            }
            if (value == -std::numeric_limits<FloatT>::infinity()) {
                return BasicString(GetNegInfStr());
            }
            
            // Simple float to BasicString conversion (production code should use snprintf/swprintf)
            bool negative = value < 0;
            if (negative) value = -value;
            
            // Integer part
            long long intPart = static_cast<long long>(value);
            FloatT fracPart = value - static_cast<FloatT>(intPart);
            
            // Build BasicString
            BasicString result = FromInt(negative ? -intPart : intPart);
            
            if (precision > 0) {
                result += GetDecimalChar();
                
                // Fractional part
                for (int i = 0; i < precision; ++i) {
                    fracPart *= 10;
                    int digit = static_cast<int>(fracPart);
                    result += GetDigitChar(digit);
                    fracPart -= digit;
                }
            }
            
            return result;
        }
        
        // Convert boolean to BasicString
        static BasicString FromBool(bool value) noexcept {
            return value ? BasicString(GetTrueStr()) : BasicString(GetFalseStr());
        }
        
        // Convert pointer to hex BasicString
        template<typename T>
        static BasicString FromPointer(T* ptr) noexcept {
            if (!ptr) {
                return BasicString(GetNullptrStr());
            }
            
            BasicString result(GetHexPrefix());
            
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            CharT buffer[32];
            CharT* bufPtr = buffer + 31;
            *bufPtr = CharT{0};
            
            // Convert to hex
            do {
                int digit = addr & 0xF;
                *(--bufPtr) = GetHexChar(digit);
                addr >>= 4;
            } while (addr > 0);
            
            result += bufPtr;
            return result;
        }
        
        // =========================================================================
        // NUMERIC CONVERSION FROM BasicString
        // =========================================================================
        
        // Convert BasicString to integer
        template<typename IntT = int>
        [[nodiscard]] IntT ToInt(bool* success = nullptr) const noexcept {
            static_assert(std::is_integral_v<IntT>, "ToInt requires integral type");
            
            if (success) *success = false;
            if (IsEmpty()) return IntT{0};
            
            const CharT* str = GetData();
            SizeType pos = 0;
            
            // Skip whitespace
            while (pos < mLength && IsWhitespace(str[pos])) ++pos;
            if (pos >= mLength) return IntT{0};
            
            // Check sign
            bool negative = false;
            if (str[pos] == GetMinusChar()) {
                if constexpr (std::is_unsigned_v<IntT>) {
                    return IntT{0}; // Unsigned can't be negative
                }
                negative = true;
                ++pos;
            } else if (str[pos] == GetPlusChar()) {
                ++pos;
            }
            
            // Parse digits
            IntT result = 0;
            bool hasDigits = false;
            
            while (pos < mLength && IsDigit(str[pos])) {
                int digit = str[pos] - GetDigitChar(0);
                result = result * 10 + digit;
                ++pos;
                hasDigits = true;
            }
            
            if (success) *success = hasDigits;
            return negative ? -result : result;
        }
        
        // Convert BasicString to float
        [[nodiscard]] float ToFloat(bool* success = nullptr) const noexcept {
            return static_cast<float>(ToDouble(success));
        }
        
        // Convert BasicString to double
        [[nodiscard]] double ToDouble(bool* success = nullptr) const noexcept {
            if (success) *success = false;
            if (IsEmpty()) return 0.0;
            
            const CharT* str = GetData();
            SizeType pos = 0;
            
            // Skip whitespace
            while (pos < mLength && IsWhitespace(str[pos])) ++pos;
            if (pos >= mLength) return 0.0;
            
            // Check for special values
            if (StartsWithAt(GetNaNStr(), pos)) {
                if (success) *success = true;
                return std::numeric_limits<double>::quiet_NaN();
            }
            if (StartsWithAt(GetInfStr(), pos)) {
                if (success) *success = true;
                return std::numeric_limits<double>::infinity();
            }
            
            // Parse sign
            bool negative = false;
            if (str[pos] == GetMinusChar()) {
                negative = true;
                ++pos;
            } else if (str[pos] == GetPlusChar()) {
                ++pos;
            }
            
            // Parse integer part
            double result = 0.0;
            bool hasDigits = false;
            
            while (pos < mLength && IsDigit(str[pos])) {
                result = result * 10.0 + (str[pos] - GetDigitChar(0));
                ++pos;
                hasDigits = true;
            }
            
            // Parse fractional part
            if (pos < mLength && str[pos] == GetDecimalChar()) {
                ++pos;
                double fraction = 0.0;
                double divisor = 10.0;
                
                while (pos < mLength && IsDigit(str[pos])) {
                    fraction += (str[pos] - GetDigitChar(0)) / divisor;
                    divisor *= 10.0;
                    ++pos;
                    hasDigits = true;
                }
                
                result += fraction;
            }
            
            if (success) *success = hasDigits;
            return negative ? -result : result;
        }
        
        // Convert BasicString to boolean
        [[nodiscard]] bool ToBool() const noexcept {
            if (IsEmpty()) return false;
            
            // Check for "true" (case-insensitive)
            if (CompareIgnoreCaseWith(GetTrueStr())) return true;
            
            // Check for "1"
            if (mLength == 1 && GetData()[0] == GetDigitChar(1)) return true;
            
            return false;
        }

    private:
        // Helper functions for character literals (compile-time selection based on CharT)
        static constexpr CharT GetDigitChar(int digit) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return '0' + static_cast<char>(digit);
            } else {
                return L'0' + static_cast<wchar_t>(digit);
            }
        }
        
        static constexpr CharT GetHexChar(int digit) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            } else {
                return (digit < 10) ? (L'0' + digit) : (L'a' + digit - 10);
            }
        }
        
        static constexpr CharT GetMinusChar() noexcept {
            return (std::is_same_v<CharT, char>) ? '-' : L'-';
        }
        
        static constexpr CharT GetPlusChar() noexcept {
            return (std::is_same_v<CharT, char>) ? '+' : L'+';
        }
        
        static constexpr CharT GetDecimalChar() noexcept {
            return (std::is_same_v<CharT, char>) ? '.' : L'.';
        }
        
        static constexpr const CharT* GetTrueStr() noexcept {
            return (std::is_same_v<CharT, char>) ? "true" : L"true";
        }
        
        static constexpr const CharT* GetFalseStr() noexcept {
            return (std::is_same_v<CharT, char>) ? "false" : L"false";
        }
        
        static constexpr const CharT* GetNaNStr() noexcept {
            return (std::is_same_v<CharT, char>) ? "nan" : L"nan";
        }
        
        static constexpr const CharT* GetInfStr() noexcept {
            return (std::is_same_v<CharT, char>) ? "inf" : L"inf";
        }
        
        static constexpr const CharT* GetNegInfStr() noexcept {
            return (std::is_same_v<CharT, char>) ? "-inf" : L"-inf";
        }
        
        static constexpr const CharT* GetNullptrStr() noexcept {
            return (std::is_same_v<CharT, char>) ? "nullptr" : L"nullptr";
        }
        
        static constexpr const CharT* GetHexPrefix() noexcept {
            return (std::is_same_v<CharT, char>) ? "0x" : L"0x";
        }
        
        static bool IsWhitespace(CharT c) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return c == ' ' || c == '\t' || c == '\n' || c == '\r';
            } else {
                return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
            }
        }
        
        static bool IsDigit(CharT c) noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return c >= '0' && c <= '9';
            } else {
                return c >= L'0' && c <= L'9';
            }
        }
        
        bool StartsWithAt(const CharT* prefix, SizeType pos) const noexcept {
            if (!prefix) return false;
            SizeType prefixLen = StrLen(prefix);
            if (pos + prefixLen > mLength) return false;
            return StrCompare(GetData() + pos, prefix, prefixLen) == 0;
        }
        
        bool CompareIgnoreCaseWith(const CharT* other) const noexcept {
            if (!other) return false;
            SizeType otherLen = StrLen(other);
            if (mLength != otherLen) return false;
            
            const CharT* data = GetData();
            for (SizeType i = 0; i < mLength; ++i) {
                if (ToLowerChar(data[i]) != ToLowerChar(other[i])) {
                    return false;
                }
            }
            return true;
        }

    public:
        // =========================================================================
        // ENCODING CONVERSION (char <-> wchar_t)
        // =========================================================================
        
        // Convert from narrow (char) BasicString to this BasicString type
        template<typename SrcCharT = char>
        static BasicString FromNarrow(const SrcCharT* str) noexcept {
            static_assert(std::is_same_v<SrcCharT, char>, "FromNarrow expects char*");
            
            if constexpr (std::is_same_v<CharT, char>) {
                // char -> char: direct copy
                return BasicString(str);
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                // char -> wchar_t: convert
                if (!str) return BasicString();
                
                SizeType len = std::strlen(str);
                BasicString result;
                
                if (len > SsoCapacity) {
                    result.AllocateHeap(len);
                }
                
                wchar_t* dest = result.GetData();
                for (SizeType i = 0; i < len; ++i) {
                    // Basic ASCII conversion (for full locale support, use mbstowcs)
                    dest[i] = static_cast<wchar_t>(static_cast<unsigned char>(str[i]));
                }
                dest[len] = wchar_t{0};
                result.mLength = len;
                
                return result;
            }
        }
        
        // Convert from wide (wchar_t) BasicString to this BasicString type
        template<typename SrcCharT = wchar_t>
        static BasicString FromWide(const SrcCharT* str) noexcept {
            static_assert(std::is_same_v<SrcCharT, wchar_t>, "FromWide expects wchar_t*");
            
            if constexpr (std::is_same_v<CharT, wchar_t>) {
                // wchar_t -> wchar_t: direct copy
                return BasicString(str);
            } else if constexpr (std::is_same_v<CharT, char>) {
                // wchar_t -> char: convert (lossy for non-ASCII)
                if (!str) return BasicString();
                
                SizeType len = std::wcslen(str);
                BasicString result;
                
                if (len > SsoCapacity) {
                    result.AllocateHeap(len);
                }
                
                char* dest = result.GetData();
                for (SizeType i = 0; i < len; ++i) {
                    // Basic truncation (for full locale support, use wcstombs)
                    // Characters > 255 will be truncated - consider this in production
                    dest[i] = static_cast<char>(str[i] & 0xFF);
                }
                dest[len] = char{0};
                result.mLength = len;
                
                return result;
            }
        }
        
        // Convert this BasicString to narrow (char) representation
        [[nodiscard]] BasicString<char, Allocator> ToNarrow() const noexcept {
            if constexpr (std::is_same_v<CharT, char>) {
                return *this;
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                BasicString<char, Allocator> result;
                
                if (mLength > BasicString<char, Allocator>::SsoCapacity) {
                    result.Reserve(mLength);
                }
                
                const wchar_t* src = GetData();
                char* dest = result.GetData();
                
                for (SizeType i = 0; i < mLength; ++i) {
                    dest[i] = static_cast<char>(src[i] & 0xFF);
                }
                dest[mLength] = char{0};
                result.mLength = mLength;
                
                return result;
            }
        }
        
        // Convert this BasicString to wide (wchar_t) representation
        [[nodiscard]] BasicString<wchar_t, Allocator> ToWide() const noexcept {
            if constexpr (std::is_same_v<CharT, wchar_t>) {
                return *this;
            } else if constexpr (std::is_same_v<CharT, char>) {
                BasicString<wchar_t, Allocator> result;
                
                if (mLength > BasicString<wchar_t, Allocator>::SsoCapacity) {
                    result.Reserve(mLength);
                }
                
                const char* src = GetData();
                wchar_t* dest = result.GetData();
                
                for (SizeType i = 0; i < mLength; ++i) {
                    dest[i] = static_cast<wchar_t>(static_cast<unsigned char>(src[i]));
                }
                dest[mLength] = wchar_t{0};
                result.mLength = mLength;
                
                return result;
            }
        }
        
        // =========================================================================
        // CONCATENATION OPERATORS
        // =========================================================================
        
        BasicString& operator+=(const CharT* str) noexcept {
            Append(str);
            return *this;
        }
        
        BasicString& operator+=(const BasicString& other) noexcept {
            Append(other);
            return *this;
        }
        
        BasicString& operator+=(CharT c) noexcept {
            CharT str[2] = {c, CharT{0}};
            Append(str);
            return *this;
        }
        
        [[nodiscard]] friend BasicString operator+(const BasicString& lhs, const BasicString& rhs) noexcept {
            BasicString result;
            result.Reserve(lhs.mLength + rhs.mLength);
            result.Append(lhs);
            result.Append(rhs);
            return result;
        }
        
        [[nodiscard]] friend BasicString operator+(const BasicString& lhs, const CharT* rhs) noexcept {
            BasicString result;
            SizeType rhsLen = rhs ? StrLen(rhs) : 0;
            result.Reserve(lhs.mLength + rhsLen);
            result.Append(lhs);
            if (rhs) result.Append(rhs);
            return result;
        }
        
        [[nodiscard]] friend BasicString operator+(const CharT* lhs, const BasicString& rhs) noexcept {
            BasicString result;
            SizeType lhsLen = lhs ? StrLen(lhs) : 0;
            result.Reserve(lhsLen + rhs.mLength);
            if (lhs) result.Append(lhs);
            result.Append(rhs);
            return result;
        }
        
        [[nodiscard]] friend BasicString operator+(const BasicString& lhs, CharT rhs) noexcept {
            BasicString result;
            result.Reserve(lhs.mLength + 1);
            result.Append(lhs);
            result.Append(CharT{rhs});
            return result;
        }
        
        [[nodiscard]] friend BasicString operator+(CharT lhs, const BasicString& rhs) noexcept {
            BasicString result;
            result.Reserve(1 + rhs.mLength);
            CharT str[2] = {lhs, CharT{0}};
            result.Append(str);
            result.Append(rhs);
            return result;
        }
        
        // =========================================================================
        // COMPARISON OPERATIONS
        // =========================================================================
        
        [[nodiscard]] bool operator==(const BasicString& other) const noexcept {
            if (mLength != other.mLength) return false;
            return StrCompare(GetData(), other.GetData(), mLength) == 0;
        }
        
        [[nodiscard]] bool operator!=(const BasicString& other) const noexcept {
            return !(*this == other);
        }
        
        [[nodiscard]] int Compare(const BasicString& other) const noexcept {
            SizeType minLen = (mLength < other.mLength) ? mLength : other.mLength;
            int result = StrCompare(GetData(), other.GetData(), minLen);
            
            if (result == 0) {
                if (mLength < other.mLength) return -1;
                if (mLength > other.mLength) return 1;
            }
            return result;
        }
        
        [[nodiscard]] int CompareIgnoreCase(const BasicString& other) const noexcept {
            SizeType minLen = (mLength < other.mLength) ? mLength : other.mLength;
            
            const CharT* data1 = GetData();
            const CharT* data2 = other.GetData();
            
            for (SizeType i = 0; i < minLen; ++i) {
                CharT c1 = ToLowerChar(data1[i]);
                CharT c2 = ToLowerChar(data2[i]);
                if (c1 != c2) return (c1 < c2) ? -1 : 1;
            }
            
            if (mLength < other.mLength) return -1;
            if (mLength > other.mLength) return 1;
            return 0;
        }
    };

    // =============================================================================
    // TYPE ALIASES FOR CONVENIENCE
    // =============================================================================

    using String = BasicString<char, DefaultAllocator<char>>;
    using StringW = BasicString<wchar_t, DefaultAllocator<wchar_t>>;
}