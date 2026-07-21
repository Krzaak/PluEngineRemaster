#pragma once
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <initializer_list>
#include "Allocators/Default.h"
#include "Random/Random.h"

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

    // Utility methods - Losowanie (PluRandom, silnik thread_local)
    // Losowy indeks; InvalidIndex gdy tablica jest pusta.
    [[nodiscard]] SizeType GetRandomIndex() const {
        if (m_Size == 0) return InvalidIndex;
        return static_cast<SizeType>(PluRandom::NextIndex(m_Size));
    }

    // Losowy element. Rzuca na pustej tablicy — użyj GetRandomItemPtr(), jeśli
    // pustka jest normalnym przypadkiem.
    T& GetRandomItem() {
        if (m_Size == 0) throw std::out_of_range("GetRandomItem on empty array");
        return m_Data[PluRandom::NextIndex(m_Size)];
    }

    const T& GetRandomItem() const {
        if (m_Size == 0) throw std::out_of_range("GetRandomItem on empty array");
        return m_Data[PluRandom::NextIndex(m_Size)];
    }

    // Wersja bez wyjątku — nullptr gdy pusto.
    T* GetRandomItemPtr() {
        return m_Size == 0 ? nullptr : &m_Data[PluRandom::NextIndex(m_Size)];
    }

    const T* GetRandomItemPtr() const {
        return m_Size == 0 ? nullptr : &m_Data[PluRandom::NextIndex(m_Size)];
    }

    // Losowy element spełniający predykat (reservoir sampling — jeden przebieg,
    // bez alokacji). nullptr gdy nic nie pasuje.
    template<typename Predicate>
    T* GetRandomItemIf(Predicate pred) {
        T* picked = nullptr;
        SizeType matches = 0;
        for (SizeType i = 0; i < m_Size; ++i) {
            if (!pred(m_Data[i])) continue;
            ++matches;
            if (PluRandom::NextIndex(matches) == 0) picked = &m_Data[i];
        }
        return picked;
    }

    // Tasowanie Fisher-Yates in-place.
    void Shuffle() {
        for (SizeType i = m_Size; i > 1; --i) {
            const SizeType j = static_cast<SizeType>(PluRandom::NextIndex(i));
            SwapItems(i - 1, j);
        }
    }

    // Utility methods - Szybkie usuwanie (O(1), NIE zachowuje kolejności)
    void RemoveAtSwap(SizeType index) {
        if (index >= m_Size) throw std::out_of_range("Index out of range");
        if (index != m_Size - 1) {
            m_Data[index] = std::move(m_Data[m_Size - 1]);
        }
        PopBack();
    }

    bool RemoveSwap(const T& value) {
        const SizeType index = IndexOf(value);
        if (index == InvalidIndex) return false;
        RemoveAtSwap(index);
        return true;
    }

    // Utility methods - Zapytania
    static constexpr SizeType InvalidIndex = static_cast<SizeType>(-1);

    [[nodiscard]] bool IsValidIndex(SizeType index) const { return index < m_Size; }

    template<typename Predicate>
    SizeType IndexOfIf(Predicate pred) const {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (pred(m_Data[i])) return i;
        }
        return InvalidIndex;
    }

    template<typename Predicate>
    bool ContainsIf(Predicate pred) const { return IndexOfIf(pred) != InvalidIndex; }

    template<typename Predicate>
    bool Any(Predicate pred) const { return IndexOfIf(pred) != InvalidIndex; }

    template<typename Predicate>
    bool All(Predicate pred) const {
        for (SizeType i = 0; i < m_Size; ++i) {
            if (!pred(m_Data[i])) return false;
        }
        return true;
    }

    template<typename Predicate>
    SizeType CountIf(Predicate pred) const {
        SizeType count = 0;
        for (SizeType i = 0; i < m_Size; ++i) {
            if (pred(m_Data[i])) ++count;
        }
        return count;
    }

    SizeType Count(const T& value) const {
        SizeType count = 0;
        for (SizeType i = 0; i < m_Size; ++i) {
            if (m_Data[i] == value) ++count;
        }
        return count;
    }

    // Min/Max wg komparatora "mniejszości"; End() gdy pusto.
    template<typename Comparator>
    Iterator MinElement(Comparator comp) {
        if (m_Size == 0) return End();
        SizeType best = 0;
        for (SizeType i = 1; i < m_Size; ++i) {
            if (comp(m_Data[i], m_Data[best])) best = i;
        }
        return &m_Data[best];
    }

    template<typename Comparator>
    Iterator MaxElement(Comparator comp) {
        if (m_Size == 0) return End();
        SizeType best = 0;
        for (SizeType i = 1; i < m_Size; ++i) {
            if (comp(m_Data[best], m_Data[i])) best = i;
        }
        return &m_Data[best];
    }

    Iterator MinElement() { return MinElement([](const T& a, const T& b) { return a < b; }); }
    Iterator MaxElement() { return MaxElement([](const T& a, const T& b) { return a < b; }); }

    // Suma elementów; R pozwala uniknąć przepełnienia (np. Sum<UInt64>()).
    template<typename R = T>
    R Sum() const {
        R total{};
        for (SizeType i = 0; i < m_Size; ++i) total += static_cast<R>(m_Data[i]);
        return total;
    }

    // Utility methods - Modyfikacja
    // Dodaje tylko jeśli elementu jeszcze nie ma; true = dodano.
    bool AddUnique(const T& value) {
        if (Contains(value)) return false;
        PushBack(value);
        return true;
    }

    void SwapItems(SizeType a, SizeType b) {
        if (a == b || a >= m_Size || b >= m_Size) return;
        T temp = std::move(m_Data[a]);
        m_Data[a] = std::move(m_Data[b]);
        m_Data[b] = std::move(temp);
    }

    void Swap(DynamicArray& other) noexcept {
        std::swap(m_Data, other.m_Data);
        std::swap(m_Size, other.m_Size);
        std::swap(m_Capacity, other.m_Capacity);
        std::swap(m_Allocator, other.m_Allocator);
    }

    // Nadpisuje wszystkie istniejące elementy (nie zmienia rozmiaru).
    void Fill(const T& value) {
        for (SizeType i = 0; i < m_Size; ++i) m_Data[i] = value;
    }

    // Utility methods - Transformacje (zwracają nowe tablice)
    template<typename Predicate>
    DynamicArray Filter(Predicate pred) const {
        DynamicArray result(m_Allocator);
        for (SizeType i = 0; i < m_Size; ++i) {
            if (pred(m_Data[i])) result.PushBack(m_Data[i]);
        }
        return result;
    }

    // Mapowanie 1:1; typ wyniku wyprowadzany z funkcji.
    template<typename Func>
    auto Map(Func func) const -> DynamicArray<decltype(func(std::declval<const T&>()))> {
        DynamicArray<decltype(func(std::declval<const T&>()))> result;
        result.Reserve(m_Size);
        for (SizeType i = 0; i < m_Size; ++i) result.PushBack(func(m_Data[i]));
        return result;
    }

    // Składanie do jednej wartości: acc = func(acc, item) po kolei od początku.
    // Typ akumulatora bierze się z `init`, więc np. Reduce(String(), ...) scala stringi.
    template<typename R, typename Func>
    R Reduce(R init, Func func) const {
        R acc = std::move(init);
        for (SizeType i = 0; i < m_Size; ++i) acc = func(std::move(acc), m_Data[i]);
        return acc;
    }

    // Kopia n pierwszych / ostatnich elementów; n większe od rozmiaru = cała tablica.
    DynamicArray First(SizeType count) const {
        return Slice(0, count);
    }

    DynamicArray Last(SizeType count) const {
        if (count >= m_Size) return Slice(0);
        return Slice(m_Size - count);
    }

    // Kopia podzakresu; wychodzenie poza koniec jest przycinane, nie rzuca.
    DynamicArray Slice(SizeType start, SizeType count = InvalidIndex) const {
        DynamicArray result(m_Allocator);
        if (start >= m_Size) return result;

        const SizeType available = m_Size - start;
        const SizeType take = count < available ? count : available;
        result.Reserve(take);
        for (SizeType i = 0; i < take; ++i) result.PushBack(m_Data[start + i]);
        return result;
    }

    // Porównania
    bool operator==(const DynamicArray& other) const {
        if (m_Size != other.m_Size) return false;
        for (SizeType i = 0; i < m_Size; ++i) {
            if (!(m_Data[i] == other.m_Data[i])) return false;
        }
        return true;
    }

    bool operator!=(const DynamicArray& other) const { return !(*this == other); }

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