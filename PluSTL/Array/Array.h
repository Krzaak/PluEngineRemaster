#pragma once
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <initializer_list>
#include "Allocators/Default.h"

template<typename T, typename Allocator = DefaultAllocator<T>>
class DynamicArray {
public:
    using ValueType = T;
    using SizeType = size_t;
    using Iterator = T*;
    using ConstIterator = const T*;
    using AllocatorType = Allocator;

    // Konstruktory z obsługą instancji alokatora
    explicit DynamicArray(const Allocator& alloc = Allocator())
        : m_Data(nullptr), m_Size(0), m_Capacity(0), m_Allocator(alloc) {}

    explicit DynamicArray(SizeType capacity, const Allocator& alloc = Allocator())
        : m_Data(nullptr), m_Size(0), m_Capacity(0), m_Allocator(alloc) {
        Reserve(capacity);
    }

    DynamicArray(std::initializer_list<T> init, const Allocator& alloc = Allocator())
        : m_Data(nullptr), m_Size(0), m_Capacity(0), m_Allocator(alloc) {
        Reserve(init.size());
        for (const auto& item : init) {
            PushBack(item);
        }
    }

    // Copy constructor - kopiuje również alokator
    DynamicArray(const DynamicArray& other)
        : m_Data(nullptr), m_Size(0), m_Capacity(0), m_Allocator(other.m_Allocator) {
        Reserve(other.m_Size);
        for (SizeType i = 0; i < other.m_Size; ++i) {
            m_Allocator.Construct(&m_Data[i], other.m_Data[i]);
        }
        m_Size = other.m_Size;
    }

    // Move constructor
    DynamicArray(DynamicArray&& other) noexcept
        : m_Data(other.m_Data), m_Size(other.m_Size), m_Capacity(other.m_Capacity),
          m_Allocator(std::move(other.m_Allocator)) {
        other.m_Data = nullptr;
        other.m_Size = 0;
        other.m_Capacity = 0;
    }

    // Destruktor - używa instancji alokatora
    ~DynamicArray() {
        Clear();
        if (m_Data) {
            m_Allocator.Deallocate(m_Data, m_Capacity);
        }
    }

    // Copy assignment
    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            Clear();
            if (m_Data) {
                m_Allocator.Deallocate(m_Data, m_Capacity);
                m_Data = nullptr;
                m_Capacity = 0;
            }

            m_Allocator = other.m_Allocator;
            Reserve(other.m_Size);
            for (SizeType i = 0; i < other.m_Size; ++i) {
                m_Allocator.Construct(&m_Data[i], other.m_Data[i]);
            }
            m_Size = other.m_Size;
        }
        return *this;
    }

    // Move assignment
    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            Clear();
            if (m_Data) {
                m_Allocator.Deallocate(m_Data, m_Capacity);
            }

            m_Data = other.m_Data;
            m_Size = other.m_Size;
            m_Capacity = other.m_Capacity;
            m_Allocator = std::move(other.m_Allocator);

            other.m_Data = nullptr;
            other.m_Size = 0;
            other.m_Capacity = 0;
        }
        return *this;
    }

    // Dodawanie elementów - używa m_Allocator.Construct
    void PushBack(const T& value) {
        if (m_Size >= m_Capacity) {
            Reserve(m_Capacity == 0 ? 2 : m_Capacity * 2);
        }
        m_Allocator.Construct(&m_Data[m_Size], value);
        ++m_Size;
    }

    void PushBack(T&& value) {
        if (m_Size >= m_Capacity) {
            Reserve(m_Capacity == 0 ? 2 : m_Capacity * 2);
        }
        m_Allocator.Construct(&m_Data[m_Size], std::move(value));
        ++m_Size;
    }

    template<typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (m_Size >= m_Capacity) {
            Reserve(m_Capacity == 0 ? 2 : m_Capacity * 2);
        }
        m_Allocator.Construct(&m_Data[m_Size], std::forward<Args>(args)...);
        return m_Data[m_Size++];
    }

    void PopBack() {
        if (m_Size > 0) {
            --m_Size;
            m_Allocator.Destroy(&m_Data[m_Size]);
        }
    }

    // Dostęp do elementów
    T& operator[](SizeType index) { return m_Data[index]; }
    const T& operator[](SizeType index) const { return m_Data[index]; }

    T& At(SizeType index) {
        if (index >= m_Size) throw std::out_of_range("Index out of range");
        return m_Data[index];
    }

    const T& At(SizeType index) const {
        if (index >= m_Size) throw std::out_of_range("Index out of range");
        return m_Data[index];
    }

    T& Front() { return m_Data[0]; }
    const T& Front() const { return m_Data[0]; }
    T& Back() { return m_Data[m_Size - 1]; }
    const T& Back() const { return m_Data[m_Size - 1]; }
    T* Data() { return m_Data; }
    const T* Data() const { return m_Data; }

    // Rozmiar i pojemność
    [[nodiscard]] SizeType Size() const { return m_Size; }
    [[nodiscard]] SizeType Capacity() const { return m_Capacity; }
    [[nodiscard]] bool IsEmpty() const { return m_Size == 0; }

    void Reserve(SizeType newCapacity) {
        if (newCapacity <= m_Capacity) return;

        T* newData = m_Allocator.Allocate(newCapacity);

        for (SizeType i = 0; i < m_Size; ++i) {
            m_Allocator.Construct(&newData[i], std::move(m_Data[i]));
            m_Allocator.Destroy(&m_Data[i]);
        }

        if (m_Data) {
            m_Allocator.Deallocate(m_Data, m_Capacity);
        }

        m_Data = newData;
        m_Capacity = newCapacity;
    }

    void Resize(SizeType newSize) {
        if (newSize > m_Capacity) {
            Reserve(newSize);
        }

        if (newSize > m_Size) {
            for (SizeType i = m_Size; i < newSize; ++i) {
                m_Allocator.Construct(&m_Data[i]);
            }
        } else {
            for (SizeType i = newSize; i < m_Size; ++i) {
                m_Allocator.Destroy(&m_Data[i]);
            }
        }

        m_Size = newSize;
    }

    void ShrinkToFit() {
        if (m_Size < m_Capacity) {
            T* newData = m_Size > 0 ? m_Allocator.Allocate(m_Size) : nullptr;

            for (SizeType i = 0; i < m_Size; ++i) {
                m_Allocator.Construct(&newData[i], std::move(m_Data[i]));
                m_Allocator.Destroy(&m_Data[i]);
            }

            if (m_Data) {
                m_Allocator.Deallocate(m_Data, m_Capacity);
            }

            m_Data = newData;
            m_Capacity = m_Size;
        }
    }

    void Clear() {
        for (SizeType i = 0; i < m_Size; ++i) {
            m_Allocator.Destroy(&m_Data[i]);
        }
        m_Size = 0;
    }

    // Iteratory
    Iterator Begin() { return m_Data; }
    ConstIterator Begin() const { return m_Data; }
    Iterator End() { return m_Data + m_Size; }
    ConstIterator End() const { return m_Data + m_Size; }

    Iterator begin() { return Begin(); }
    ConstIterator begin() const { return Begin(); }
    Iterator end() { return End(); }
    ConstIterator end() const { return End(); }

    // Utility methods - Wyszukiwanie
    Iterator Find(const T& value) {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (m_Data[i] == value) return &m_Data[i];
        }
        return End();
    }

    ConstIterator Find(const T& value) const {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (m_Data[i] == value) return &m_Data[i];
        }
        return End();
    }

    template<typename Predicate>
    Iterator FindIf(Predicate pred) {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (pred(m_Data[i])) return &m_Data[i];
        }
        return End();
    }

    template<typename Predicate>
    ConstIterator FindIf(Predicate pred) const {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (pred(m_Data[i])) return &m_Data[i];
        }
        return End();
    }

    bool Contains(const T& value) const {
        return Find(value) != End();
    }

    SizeType IndexOf(const T& value) const {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (m_Data[i] == value) return i;
        }
        return static_cast<SizeType>(-1);
    }

    // Utility methods - Usuwanie
    bool Remove(const T& value) {
        Iterator it = Find(value);
        if (it != End()) {
            Erase(it);
            return true;
        }
        return false;
    }

    template<typename Predicate>
    SizeType RemoveIf(Predicate pred) {
        SizeType removed = 0;
        for (SizeType i = 0; i < m_Size;) {
            if (pred(m_Data[i])) {
                Erase(&m_Data[i]);
                ++removed;
            } else {
                ++i;
            }
        }
        return removed;
    }

    void RemoveAt(SizeType index) {
        if (index >= m_Size) throw std::out_of_range("Index out of range");
        Erase(&m_Data[index]);
    }

    void Erase(Iterator it) {
        if (it < Begin() || it >= End()) return;

        SizeType index = it - Begin();
        m_Allocator.Destroy(&m_Data[index]);

        for (SizeType i = index; i < m_Size - 1; ++i) {
            m_Allocator.Construct(&m_Data[i], std::move(m_Data[i + 1]));
            m_Allocator.Destroy(&m_Data[i + 1]);
        }

        --m_Size;
    }

    void Erase(Iterator first, Iterator last) {
        if (first >= last || first < Begin() || last > End()) return;

        SizeType startIdx = first - Begin();
        SizeType endIdx = last - Begin();
        SizeType count = endIdx - startIdx;

        for (SizeType i = startIdx; i < endIdx; ++i) {
            m_Allocator.Destroy(&m_Data[i]);
        }

        for (SizeType i = startIdx; i < m_Size - count; ++i) {
            m_Allocator.Construct(&m_Data[i], std::move(m_Data[i + count]));
            m_Allocator.Destroy(&m_Data[i + count]);
        }

        m_Size -= count;
    }

    // Utility methods - Modyfikacja
    void Insert(Iterator pos, const T& value) {
        SizeType index = pos - Begin();
        if (m_Size >= m_Capacity) {
            Reserve(m_Capacity == 0 ? 2 : m_Capacity * 2);
        }

        for (SizeType i = m_Size; i > index; --i) {
            m_Allocator.Construct(&m_Data[i], std::move(m_Data[i - 1]));
            m_Allocator.Destroy(&m_Data[i - 1]);
        }

        m_Allocator.Construct(&m_Data[index], value);
        ++m_Size;
    }

    void Insert(Iterator pos, T&& value) {
        SizeType index = pos - Begin();
        if (m_Size >= m_Capacity) {
            Reserve(m_Capacity == 0 ? 2 : m_Capacity * 2);
        }

        for (SizeType i = m_Size; i > index; --i) {
            m_Allocator.Construct(&m_Data[i], std::move(m_Data[i - 1]));
            m_Allocator.Destroy(&m_Data[i - 1]);
        }

        m_Allocator.Construct(&m_Data[index], std::move(value));
        ++m_Size;
    }

    void Reverse() {
        for (SizeType i = 0; i < m_Size / 2; ++i) {
            T temp = std::move(m_Data[i]);
            m_Data[i] = std::move(m_Data[m_Size - 1 - i]);
            m_Data[m_Size - 1 - i] = std::move(temp);
        }
    }

    template<typename Comparator>
    void Sort(Comparator comp) {
        if (m_Size <= 1) return;
        QuickSort(0, m_Size - 1, comp);
    }

    void Sort() {
        Sort([](const T& a, const T& b) { return a < b; });
    }

    // Dodaje elementy z innej tablicy DynamicArray
    void Append(const DynamicArray& other) {
        if (other.IsEmpty()) return;

        Reserve(m_Size + other.m_Size);
        for (const auto& item : other) {
            m_Allocator.Construct(&m_Data[m_Size], item);
            ++m_Size;
        }
    }

    // Dodaje elementy z innej tablicy DynamicArray (wersja Move)
    void Append(DynamicArray&& other) {
        if (other.IsEmpty()) return;

        Reserve(m_Size + other.m_Size);
        for (auto& item : other) {
            m_Allocator.Construct(&m_Data[m_Size], std::move(item));
            ++m_Size;
        }
        // Opcjonalnie czyścimy źródło, choć destruktor 'other' i tak by to zrobił
        other.m_Size = 0;
    }

    // Dodaje listę inicjalizacyjną, np. arr.Append({1, 2, 3});
    void Append(std::initializer_list<T> init) {
        if (init.size() == 0) return;

        Reserve(m_Size + init.size());
        for (const auto& item : init) {
            m_Allocator.Construct(&m_Data[m_Size], item);
            ++m_Size;
        }
    }

private:
    T* m_Data;
    SizeType m_Size;
    SizeType m_Capacity;
    [[no_unique_address]] Allocator m_Allocator;

    template<typename Comparator>
    void QuickSort(SizeType low, SizeType high, Comparator comp) {
        if (low < high) {
            SizeType pi = Partition(low, high, comp);
            if (pi > 0) QuickSort(low, pi - 1, comp);
            QuickSort(pi + 1, high, comp);
        }
    }

    template<typename Comparator>
    SizeType Partition(SizeType low, SizeType high, Comparator comp) {
        T& pivot = m_Data[high];
        SizeType i = low;

        for (SizeType j = low; j < high; ++j) {
            if (comp(m_Data[j], pivot)) {
                T temp = std::move(m_Data[i]);
                m_Data[i] = std::move(m_Data[j]);
                m_Data[j] = std::move(temp);
                ++i;
            }
        }

        T temp = std::move(m_Data[i]);
        m_Data[i] = std::move(m_Data[high]);
        m_Data[high] = std::move(temp);

        return i;
    }
};