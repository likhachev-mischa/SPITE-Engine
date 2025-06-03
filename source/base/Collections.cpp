#include "Collections.hpp"

#include "Assert.hpp"

#include "memory/HeapAllocator.hpp"

namespace spite
{
	template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>::Storage::Storage(): heapPtr(nullptr)
    {}

    template <typename T, size_t InlineCapacity>
    T* sbo_vector<T, InlineCapacity>::data() noexcept
    {
        return m_isInline ? reinterpret_cast<T*>(m_storage.inlineBuffer) : m_storage.heapPtr;
    }

    template <typename T, size_t InlineCapacity>
    const T* sbo_vector<T, InlineCapacity>::data() const noexcept
    {
        return m_isInline ? reinterpret_cast<const T*>(m_storage.inlineBuffer) : m_storage.heapPtr;
    }

    template <typename T, size_t InlineCapacity>
    void sbo_vector<T, InlineCapacity>::grow_to_accommodate(size_t new_size)
    {
        if (new_size <= m_capacity) return;

        size_t new_capacity = std::max(new_size, m_capacity * 2);
        T* new_data = static_cast<T*>(getGlobalAllocator().allocate(new_capacity * sizeof(T), alignof(T)));

        // Move existing elements
        T* old_data = data();
        for (size_t i = 0; i < m_size; ++i) {
            new (new_data + i) T(std::move(old_data[i]));
            old_data[i].~T();
        }

        // Clean up old storage if it was heap-allocated
        if (!m_isInline) {
            getGlobalAllocator().deallocate(m_storage.heapPtr);
        }

        m_storage.heapPtr = new_data;
        m_capacity = new_capacity;
        m_isInline = false;
    }

    template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>::sbo_vector() = default;

    template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>::sbo_vector(std::initializer_list<T> init)
    {
        reserve(init.size());
        for (const auto& item : init) {
            push_back(item);
        }
    }

    template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>::~sbo_vector()
    {
        clear();
        if (!m_isInline) {
            getGlobalAllocator().deallocate(m_storage.heapPtr);
        }
    }

    template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>::sbo_vector(const sbo_vector& other)
    {
        reserve(other.m_size);
        for (size_t i = 0; i < other.m_size; ++i) {
            push_back(other[i]);
        }
    }

    template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>::sbo_vector(sbo_vector&& other) noexcept
    {
        if (other.m_isInline) {
            // Move elements from inline storage
            for (size_t i = 0; i < other.m_size; ++i) {
                new (data() + i) T(std::move(other.data()[i]));
                other.data()[i].~T();
            }
            m_size = other.m_size;
            other.m_size = 0;
        } else {
            // Take ownership of heap storage
            m_storage.heapPtr = other.m_storage.heapPtr;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_isInline = false;
                
            other.m_storage.heapPtr = nullptr;
            other.m_size = 0;
            other.m_capacity = InlineCapacity;
            other.m_isInline = true;
        }
    }

    template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>& sbo_vector<T, InlineCapacity>::operator=(const sbo_vector& other)
    {
        if (this != &other) {
            clear();
            reserve(other.m_size);
            for (size_t i = 0; i < other.m_size; ++i) {
                push_back(other[i]);
            }
        }
        return *this;
    }

    template <typename T, size_t InlineCapacity>
    sbo_vector<T, InlineCapacity>& sbo_vector<T, InlineCapacity>::operator=(
        sbo_vector&& other) noexcept
    {
        if (this != &other) {
            clear();
            if (!m_isInline) {
                getGlobalAllocator().deallocate(m_storage.heapPtr);
            }
                
            if (other.m_isInline) {
                // Move elements from inline storage
                m_isInline = true;
                m_capacity = InlineCapacity;
                for (size_t i = 0; i < other.m_size; ++i) {
                    new (data() + i) T(std::move(other.data()[i]));
                    other.data()[i].~T();
                }
                m_size = other.m_size;
                other.m_size = 0;
            } else {
                // Take ownership of heap storage
                m_storage.heapPtr = other.m_storage.heapPtr;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
                m_isInline = false;
                    
                other.m_storage.heapPtr = nullptr;
                other.m_size = 0;
                other.m_capacity = InlineCapacity;
                other.m_isInline = true;
            }
        }
        return *this;
    }

    template <typename T, size_t InlineCapacity>
    T& sbo_vector<T, InlineCapacity>::operator[](size_t index) noexcept
    { return data()[index]; }

    template <typename T, size_t InlineCapacity>
    const T& sbo_vector<T, InlineCapacity>::operator[](size_t index) const noexcept
    { return data()[index]; }

    template <typename T, size_t InlineCapacity>
    T& sbo_vector<T, InlineCapacity>::at(size_t index)
    {
        SASSERTM(index < m_size, "SboVector Index out of range\n")
        return data()[index];
    }

    template <typename T, size_t InlineCapacity>
    const T& sbo_vector<T, InlineCapacity>::at(size_t index) const
    {
        SASSERTM(index < m_size, "SboVector Index out of range\n")
        return data()[index];
    }

    template <typename T, size_t InlineCapacity>
    T& sbo_vector<T, InlineCapacity>::front() noexcept
    { return data()[0]; }

    template <typename T, size_t InlineCapacity>
    const T& sbo_vector<T, InlineCapacity>::front() const noexcept
    { return data()[0]; }

    template <typename T, size_t InlineCapacity>
    T& sbo_vector<T, InlineCapacity>::back() noexcept
    { return data()[m_size - 1]; }

    template <typename T, size_t InlineCapacity>
    const T& sbo_vector<T, InlineCapacity>::back() const noexcept
    { return data()[m_size - 1]; }

    template <typename T, size_t InlineCapacity>
    typename sbo_vector<T, InlineCapacity>::iterator sbo_vector<T, InlineCapacity>::begin() noexcept
    { return data(); }

    template <typename T, size_t InlineCapacity>
    typename sbo_vector<T, InlineCapacity>::const_iterator sbo_vector<T, InlineCapacity>::
    begin() const noexcept
    { return data(); }

    template <typename T, size_t InlineCapacity>
    typename sbo_vector<T, InlineCapacity>::const_iterator sbo_vector<T, InlineCapacity>::
    cbegin() const noexcept
    { return data(); }

    template <typename T, size_t InlineCapacity>
    typename sbo_vector<T, InlineCapacity>::iterator sbo_vector<T, InlineCapacity>::end() noexcept
    { return data() + m_size; }

    template <typename T, size_t InlineCapacity>
    typename sbo_vector<T, InlineCapacity>::const_iterator sbo_vector<T, InlineCapacity>::
    end() const noexcept
    { return data() + m_size; }

    template <typename T, size_t InlineCapacity>
    typename sbo_vector<T, InlineCapacity>::const_iterator sbo_vector<T, InlineCapacity>::
    cend() const noexcept
    { return data() + m_size; }

    template <typename T, size_t InlineCapacity>
    bool sbo_vector<T, InlineCapacity>::empty() const noexcept
    { return m_size == 0; }

    template <typename T, size_t InlineCapacity>
    size_t sbo_vector<T, InlineCapacity>::size() const noexcept
    { return m_size; }

    template <typename T, size_t InlineCapacity>
    size_t sbo_vector<T, InlineCapacity>::capacity() const noexcept
    { return m_capacity; }

    template <typename T, size_t InlineCapacity>
    void sbo_vector<T, InlineCapacity>::reserve(size_t new_capacity)
    {
        if (new_capacity > m_capacity) {
            grow_to_accommodate(new_capacity);
        }
    }

    template <typename T, size_t InlineCapacity>
    void sbo_vector<T, InlineCapacity>::clear() noexcept
    {
        for (size_t i = 0; i < m_size; ++i) {
            data()[i].~T();
        }
        m_size = 0;
    }

    template <typename T, size_t InlineCapacity>
    void sbo_vector<T, InlineCapacity>::push_back(const T& value)
    {
        if (m_size >= m_capacity) {
            grow_to_accommodate(m_size + 1);
        }
        new (data() + m_size) T(value);
        ++m_size;
    }

    template <typename T, size_t InlineCapacity>
    void sbo_vector<T, InlineCapacity>::push_back(T&& value)
    {
        if (m_size >= m_capacity) {
            grow_to_accommodate(m_size + 1);
        }
        new (data() + m_size) T(std::move(value));
        ++m_size;
    }

    template <typename T, size_t InlineCapacity>
    template <typename ... Args>
    T& sbo_vector<T, InlineCapacity>::emplace_back(Args&&... args)
    {
        if (m_size >= m_capacity) {
            grow_to_accommodate(m_size + 1);
        }
        T* new_element = new (data() + m_size) T(std::forward<Args>(args)...);
        ++m_size;
        return *new_element;
    }

    template <typename T, size_t InlineCapacity>
    void sbo_vector<T, InlineCapacity>::pop_back() noexcept
    {
        if (m_size > 0) {
            --m_size;
            data()[m_size].~T();
        }
    }

    template <typename T, size_t InlineCapacity>
    bool sbo_vector<T, InlineCapacity>::operator==(const sbo_vector& other) const
    {
        if (m_size != other.m_size) return false;
        for (size_t i = 0; i < m_size; ++i) {
            if (!(data()[i] == other.data()[i])) return false;
        }
        return true;
    }

    template <typename T, size_t InlineCapacity>
    bool sbo_vector<T, InlineCapacity>::operator!=(const sbo_vector& other) const
    {
        return !(*this == other);
    }

    template <typename T, size_t InlineCapacity>
    bool sbo_vector<T, InlineCapacity>::operator<(const sbo_vector& other) const
    {
        return std::lexicographical_compare(begin(), end(), other.begin(), other.end());
    }
}
