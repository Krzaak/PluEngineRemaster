#pragma once
#include <stdexcept>

namespace Plu
{
    template<typename T>
    struct ControlBlock;
    
    template<typename T>
    class TOwningPointer
    {
        ControlBlock<T>* control;

        template<typename U>
        friend class TOwningPointer;
        
        template<typename U>
        friend class TUsePointer;

        // Friend functions dla castów
        template<typename To, typename From>
        friend TOwningPointer<To> DynamicCast(const TOwningPointer<From>& from);
        
        template<typename To, typename From>
        friend TOwningPointer<To> StaticCast(const TOwningPointer<From>& from);

    public:
        // Konstruktor domyślny
        TOwningPointer() : control(nullptr) {}
        
        // Konstruktor z nullptr
        TOwningPointer(std::nullptr_t) : control(nullptr) {}
        
        // Konstruktor z surowym wskaźnikiem
        explicit TOwningPointer(T* ptr) : control(ptr ? new ControlBlock<T>(ptr) : nullptr) {}
        
        // Destruktor - zmniejsza licznik ownerów
        ~TOwningPointer()
        {
            Release();
        }

        // Copy constructor - zwiększa licznik ownerów
        TOwningPointer(const TOwningPointer& other) : control(other.control)
        {
            if (control)
            {
                control->owningCount++;
            }
        }

        // Copy constructor z konwersją (Derived -> Base)
        template<typename U>
        TOwningPointer(const TOwningPointer<U>& other) : control(reinterpret_cast<ControlBlock<T>*>(other.control))
        {
            static_assert(std::is_base_of<T, U>::value || std::is_same<T, U>::value || std::is_base_of<U, T>::value, 
                "Types must be related through inheritance or be the same type");
            if (control)
            {
                control->owningCount++;
            }
        }

        // Copy assignment
        TOwningPointer& operator=(const TOwningPointer& other)
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                if (control)
                {
                    control->owningCount++;
                }
            }
            return *this;
        }

        // Copy assignment z konwersją (Derived -> Base)
        template<typename U>
        TOwningPointer& operator=(const TOwningPointer<U>& other)
        {
            static_assert(std::is_base_of<T, U>::value || std::is_same<T, U>::value || std::is_base_of<U, T>::value, 
                "Types must be related through inheritance or be the same type");
            Release();
            control = reinterpret_cast<ControlBlock<T>*>(other.control);
            if (control)
            {
                control->owningCount++;
            }
            return *this;
        }

        // Move constructor
        TOwningPointer(TOwningPointer&& other) noexcept : control(other.control)
        {
            other.control = nullptr;
        }

        // Move assignment
        TOwningPointer& operator=(TOwningPointer&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                control = other.control;
                other.control = nullptr;
            }
            return *this;
        }

        // Przypisanie surowego wskaźnika
        TOwningPointer& operator=(T* ptr)
        {
            Release();
            control = ptr ? new ControlBlock<T>(ptr) : nullptr;
            return *this;
        }

        // Przypisanie nullptr
        TOwningPointer& operator=(std::nullptr_t)
        {
            Release();
            control = nullptr;
            return *this;
        }

        // Gettery
        [[nodiscard]] T* GetRaw() const
        {
            return control ? control->ptr : nullptr;
        }

        [[nodiscard]] T* Get() const
        {
            return GetRaw();
        }

        // Operatory
        [[nodiscard]] T* operator->() const
        {
            return GetRaw();
        }

        [[nodiscard]] T& operator*() const
        {
            return *GetRaw();
        }

        [[nodiscard]] operator bool() const
        {
            return control != nullptr && control->ptr != nullptr;
        }

        [[nodiscard]] bool operator==(const TOwningPointer& other) const
        {
            return control == other.control;
        }

        [[nodiscard]] bool operator!=(const TOwningPointer& other) const
        {
            return control != other.control;
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
                control->owningCount--;
                
                // Jeśli nie ma już żadnych ownerów, usuwamy obiekt
                if (control->owningCount == 0)
                {
                    delete control->ptr;
                    control->ptr = nullptr;
                    
                    // Jeśli nie ma też użytkowników, usuwamy control block
                    if (control->useCount == 0)
                    {
                        delete control;
                    }
                }
            }
            control = nullptr;
        }
    };
}

namespace std
{
    template<typename T>
    struct hash<Plu::TOwningPointer<T>>
    {
        size_t operator()(const Plu::TOwningPointer<T>& obj) const noexcept
        {
            return std::hash<void*>()(obj.GetRaw());
        }
    };
}
