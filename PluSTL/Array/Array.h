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
    SizeType Size() const { return m_Size; }
    SizeType Capacity() const { return m_Capacity; }
    bool IsEmpty() const { return m_Size == 0; }

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

    // Utility methods
    Iterator Find(const T& value) {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (m_Data[i] == value) return &m_Data[i];
        }
        return End();
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

    // ... (metody Reverse, Sort pozostają bez zmian w logice,
    // ponieważ operują na gotowych elementach wewnątrz m_Data) ...

private:
    T* m_Data;
    SizeType m_Size;
    SizeType m_Capacity;

    [[no_unique_address]] Allocator m_Allocator; // Instancja alokatora
    
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