#pragma once
#include <initializer_list>

namespace spite
{
    // Small Buffer Optimized (SBO) vector for small collections
    // Stores up to N elements inline, falls back to global allocation for larger sizes
    template<typename T, size_t InlineCapacity = 8>
    class sbo_vector
    {
    private:
        static_assert(InlineCapacity > 0, "InlineCapacity must be greater than 0");
        
        union Storage {
            alignas(T) char inlineBuffer[sizeof(T) * InlineCapacity];
            T* heapPtr;
            
            Storage();
        } m_storage;
        
        size_t m_size = 0;
        size_t m_capacity = InlineCapacity;
        bool m_isInline = true;

        T* data() noexcept;

        const T* data() const noexcept;

        void grow_to_accommodate(size_t new_size);

    public:
        using value_type = T;
        using size_type = size_t;
        using iterator = T*;
        using const_iterator = const T*;

        sbo_vector();

        sbo_vector(std::initializer_list<T> init);

        ~sbo_vector();

        // Copy constructor
        sbo_vector(const sbo_vector& other);

        // Move constructor
        sbo_vector(sbo_vector&& other) noexcept;

        // Assignment operators
        sbo_vector& operator=(const sbo_vector& other);

        sbo_vector& operator=(sbo_vector&& other) noexcept;

        T& operator[](size_t index) noexcept;
        const T& operator[](size_t index) const noexcept;

        T& at(size_t index);

        const T& at(size_t index) const;

        T& front() noexcept;
        const T& front() const noexcept;

        T& back() noexcept;
        const T& back() const noexcept;

        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        bool empty() const noexcept;
        size_t size() const noexcept;
        size_t capacity() const noexcept;

        void reserve(size_t new_capacity);

        // Modifiers
        void clear() noexcept;

        void push_back(const T& value);

        void push_back(T&& value);

        template<typename... Args>
        T& emplace_back(Args&&... args);

        void pop_back() noexcept;

        bool operator==(const sbo_vector& other) const;

        bool operator!=(const sbo_vector& other) const;

        bool operator<(const sbo_vector& other) const;
    };

    
}
