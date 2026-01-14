#pragma once
#include <stdexcept>

namespace Plu
{
    template<typename T>
    class TOwningPointer;

    template<typename T>
    struct ControlBlock;
    
    template<typename T>
    class TUsePointer
    {
        ControlBlock<T>* control;

        template<typename U>
        friend class TUsePointer;

        // Friend functions dla castów
        template<typename To, typename From>
        friend TUsePointer<To> DynamicCast(const TUsePointer<From>& from);
        
        template<typename To, typename From>
        friend TUsePointer<To> StaticCast(const TUsePointer<From>& from);

    public:
        // Konstruktor domyślny
        TUsePointer() : control(nullptr) {}
        
        // Konstruktor z nullptr
        TUsePointer(std::nullptr_t) : control(nullptr) {}
        
        // Konstruktor z TOwningPointer
        TUsePointer(const TOwningPointer<T>& owner) : control(owner.control)
        {
            if (control)
            {
                control->useCount++;
            }
        }

        // Konstruktor z TOwningPointer z konwersją (Derived -> Base)
        template<typename U>
        TUsePointer(const TOwningPointer<U>& owner) : control(reinterpret_cast<ControlBlock<T>*>(owner.control))
        {
            static_assert(std::is_base_of<T, U>::value || std::is_same<T, U>::value || std::is_base_of<U, T>::value, 
                "Types must be related through inheritance or be the same type");
            if (control)
            {
                control->useCount++;
            }
        }

        // Destruktor
        ~TUsePointer()
        {
            Release();
        }

        // Copy constructor
        TUsePointer(const TUsePointer& other) : control(other.control)
        {
            if (control)
            {
                control->useCount++;
            }
        }

        // Copy constructor z konwersją (Derived -> Base)
        template<typename U>
        TUsePointer(const TUsePointer<U>& other) : control(reinterpret_cast<ControlBlock<T>*>(other.control))
        {
            static_assert(std::is_base_of<T, U>::value || std::is_same<T, U>::value || std::is_base_of<U, T>::value, 
                "Types must be related through inheritance or be the same type");
            if (control)
            {
                control->useCount++;
            }
        }

        // Copy assignment
        TUsePointer& operator=(const TUsePointer& other)
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                if (control)
                {
                    control->useCount++;
                }
            }
            return *this;
        }

        // Copy assignment z konwersją (Derived -> Base)
        template<typename U>
        TUsePointer& operator=(const TUsePointer<U>& other)
        {
            static_assert(std::is_base_of<T, U>::value || std::is_same<T, U>::value || std::is_base_of<U, T>::value, 
                "Types must be related through inheritance or be the same type");
            Release();
            control = reinterpret_cast<ControlBlock<T>*>(other.control);
            if (control)
            {
                control->useCount++;
            }
            return *this;
        }

        // Move constructor
        TUsePointer(TUsePointer&& other) noexcept : control(other.control)
        {
            other.control = nullptr;
        }

        // Move assignment
        TUsePointer& operator=(TUsePointer&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                other.control = nullptr;
            }
            return *this;
        }

        // Przypisanie z TOwningPointer
        TUsePointer& operator=(const TOwningPointer<T>& owner)
        {
            Release();
            control = owner.control;
            if (control)
            {
                control->useCount++;
            }
            return *this;
        }

        // Przypisanie z TOwningPointer z konwersją (Derived -> Base)
        template<typename U>
        TUsePointer& operator=(const TOwningPointer<U>& owner)
        {
            static_assert(std::is_base_of<T, U>::value || std::is_same<T, U>::value || std::is_base_of<U, T>::value, 
                "Types must be related through inheritance or be the same type");
            Release();
            control = reinterpret_cast<ControlBlock<T>*>(owner.control);
            if (control)
            {
                control->useCount++;
            }
            return *this;
        }

        // Przypisanie nullptr
        TUsePointer& operator=(std::nullptr_t)
        {
            Release();
            control = nullptr;
            return *this;
        }

        // Gettery - rzucają wyjątek jeśli brak ownerów
        [[nodiscard]] T* GetRaw() const
        {
            if (!control || control->owningCount == 0)
            {
                throw std::runtime_error("TUsePointer: Object has no owners!");
            }
            return control->ptr;
        }

        [[nodiscard]] T* Get() const
        {
            return GetRaw();
        }

        // Operatory - rzucają wyjątek jeśli brak ownerów
        [[nodiscard]] T* operator->() const
        {
            return GetRaw();
        }

        [[nodiscard]] T& operator*() const
        {
            return *GetRaw();
        }

        // Sprawdzenie czy obiekt jest jeszcze żywy
        [[nodiscard]] bool IsValid() const
        {
            return control && control->owningCount > 0 && control->ptr != nullptr;
        }

        [[nodiscard]] explicit operator bool() const
        {
            return IsValid();
        }

        [[nodiscard]] bool operator==(const TUsePointer& other) const
        {
            return GetRaw() == other.GetRaw();
        }

        [[nodiscard]] bool operator!=(const TUsePointer& other) const
        {
            return GetRaw() != other.GetRaw();
        }

        [[nodiscard]] bool operator==(const std::nullptr_t other) const
        {
            if (!control)
            {
                return true;
            }
            return GetRaw() == nullptr;
        }

        [[nodiscard]] bool operator!=(const std::nullptr_t other) const
        {
            if (!control)
            {
                return false;
            }
            if (control->owningCount == 0)
            {
                return false;
            }
            return GetRaw() != nullptr;
        }

        // Informacje diagnostyczne
        [[nodiscard]] int GetOwningCount() const
        {
            return control ? control->owningCount : 0;
        }

        [[nodiscard]] int GetUseCount() const
        {
            return control ? control->useCount : 0;
        }

    private:
        void Release()
        {
            if (control)
            {
                control->useCount--;
                
                // Jeśli nie ma ownerów ani użytkowników, usuwamy control block
                if (control->owningCount == 0 && control->useCount == 0)
                {
                    delete control;
                }
            }
            control = nullptr;
        }
    };
}
