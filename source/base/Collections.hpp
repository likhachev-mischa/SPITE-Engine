#pragma once
#include <initializer_list>
#include <utility>

#include "memory/HeapAllocator.hpp"

namespace spite
{
	template <typename T, sizet InlineCapacity = 8, typename Allocator = GlobalHeapAllocator<T>>
	class sbo_vector
	{
	private:
		static_assert(InlineCapacity > 0, "InlineCapacity must be greater than 0");

		union Storage
		{
			alignas(T) char inlineBuffer[sizeof(T) * InlineCapacity];
			T* heapPtr;
		} m_storage;

		sizet m_size = 0;
		sizet m_capacity = InlineCapacity;
		bool m_isInline = true;
		[[no_unique_address]] Allocator m_allocator;

		T* data_ptr() noexcept
		{
			return m_isInline ? reinterpret_cast<T*>(m_storage.inlineBuffer) : m_storage.heapPtr;
		}

		const T* data_ptr() const noexcept
		{
			return m_isInline
				       ? reinterpret_cast<const T*>(m_storage.inlineBuffer)
				       : m_storage.heapPtr;
		}

		void grow_to_accommodate(sizet new_size);
		void destroy_elements() noexcept;

	public:
		using value_type = T;
		using sizetype = sizet;
		using iterator = T*;
		using const_iterator = const T*;
		using allocator_type = Allocator;

		sbo_vector() noexcept(noexcept(Allocator()));
		explicit sbo_vector(const Allocator& alloc) noexcept;
		sbo_vector(sizet count, const T& value, const Allocator& alloc = Allocator());
		explicit sbo_vector(sizet count, const Allocator& alloc = Allocator());
		sbo_vector(std::initializer_list<T> init, const Allocator& alloc = Allocator());
		~sbo_vector();

		sbo_vector(const sbo_vector& other);
		sbo_vector(sbo_vector&& other) noexcept;
		sbo_vector& operator=(const sbo_vector& other);
		sbo_vector& operator=(sbo_vector&& other) noexcept;

		T& operator[](sizet index) noexcept;
		const T& operator[](sizet index) const noexcept;

		iterator begin() noexcept;
		const_iterator begin() const noexcept;
		const_iterator cbegin() const noexcept;

		iterator end() noexcept;
		const_iterator end() const noexcept;
		const_iterator cend() const noexcept;

		bool empty() const noexcept;
		sizet size() const noexcept;
		sizet capacity() const noexcept;

		void reserve(sizet new_capacity);
		void clear() noexcept;
		void push_back(const T& value);
		void push_back(T&& value);
		template <typename... Args>
		T& emplace_back(Args&&... args);
		void pop_back() noexcept;
		void resize(sizet new_size);
		void resize(sizet new_size, const T& value);
	};


	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>::sbo_vector() noexcept(noexcept(A())) : m_allocator()
	{
	}

	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>::sbo_vector(const A& alloc) noexcept : m_allocator(alloc)
	{
	}

	template <typename T, sizet C, typename A>
	sbo_vector<
		T, C, A>::sbo_vector(sizet count, const T& value, const A& alloc) : m_allocator(alloc)
	{
		resize(count, value);
	}

	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>::sbo_vector(sizet count, const A& alloc) : m_allocator(alloc)
	{
		resize(count);
	}

	template <typename T, sizet C, typename A>
	sbo_vector<
		T, C, A>::sbo_vector(std::initializer_list<T> init, const A& alloc) : m_allocator(alloc)
	{
		reserve(init.size());
		for (const auto& item : init)
		{
			emplace_back(item);
		}
	}

	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>::~sbo_vector()
	{
		destroy_elements();
		if (!m_isInline)
		{
			m_allocator.deallocate(m_storage.heapPtr, m_capacity);
		}
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::grow_to_accommodate(sizet new_size)
	{
		if (new_size <= m_capacity) return;

		sizet new_capacity = m_capacity * 2;
		if (new_capacity < new_size) new_capacity = new_size;

		T* new_buffer = m_allocator.allocate(new_capacity);

		for (sizet i = 0; i < m_size; ++i)
		{
			new(new_buffer + i) T(std::move_if_noexcept(data_ptr()[i]));
			(data_ptr() + i)->~T();
		}

		if (!m_isInline)
		{
			m_allocator.deallocate(m_storage.heapPtr, m_capacity);
		}

		m_storage.heapPtr = new_buffer;
		m_isInline = false;
		m_capacity = new_capacity;
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::destroy_elements() noexcept
	{
		for (sizet i = 0; i < m_size; ++i)
		{
			(data_ptr() + i)->~T();
		}
	}

	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>::sbo_vector(const sbo_vector& other) : m_allocator(other.m_allocator)
	{
		reserve(other.m_size);
		for (const auto& item : other)
		{
			push_back(item);
		}
	}

	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>::sbo_vector(sbo_vector&& other) noexcept : m_size(other.m_size),
		m_capacity(other.m_capacity), m_isInline(other.m_isInline),
		m_allocator(std::move(other.m_allocator))
	{
		if (m_isInline)
		{
			for (sizet i = 0; i < m_size; ++i)
			{
				new(data_ptr() + i) T(std::move(other.data_ptr()[i]));
			}
		}
		else
		{
			m_storage.heapPtr = other.m_storage.heapPtr;
		}
		other.m_size = 0;
		other.m_isInline = true;
		other.m_capacity = C;
	}

	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>& sbo_vector<T, C, A>::operator=(const sbo_vector& other)
	{
		if (this == &other) return *this;
		clear();
		reserve(other.m_size);
		for (const auto& item : other)
		{
			push_back(item);
		}
		return *this;
	}

	template <typename T, sizet C, typename A>
	sbo_vector<T, C, A>& sbo_vector<T, C, A>::operator=(sbo_vector&& other) noexcept
	{
		if (this == &other) return *this;
		destroy_elements();
		if (!m_isInline)
		{
			m_allocator.deallocate(m_storage.heapPtr, m_capacity);
		}
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		m_isInline = other.m_isInline;
		m_allocator = std::move(other.m_allocator);

		if (m_isInline)
		{
			for (sizet i = 0; i < m_size; ++i)
			{
				new(data_ptr() + i) T(std::move(other.data_ptr()[i]));
			}
		}
		else
		{
			m_storage.heapPtr = other.m_storage.heapPtr;
		}
		other.m_size = 0;
		other.m_isInline = true;
		other.m_capacity = C;
		return *this;
	}

	template <typename T, sizet C, typename A>
	T& sbo_vector<T, C, A>::operator[](sizet index) noexcept { return data_ptr()[index]; }

	template <typename T, sizet C, typename A>
	const T& sbo_vector<T, C, A>::operator[](sizet index) const noexcept
	{
		return data_ptr()[index];
	}

	template <typename T, sizet C, typename A>
	typename sbo_vector<T, C, A>::iterator sbo_vector<T, C, A>::begin() noexcept
	{
		return data_ptr();
	}

	template <typename T, sizet C, typename A>
	typename sbo_vector<T, C, A>::const_iterator sbo_vector<T, C, A>::begin() const noexcept
	{
		return data_ptr();
	}

	template <typename T, sizet C, typename A>
	typename sbo_vector<T, C, A>::const_iterator sbo_vector<T, C, A>::cbegin() const noexcept
	{
		return data_ptr();
	}

	template <typename T, sizet C, typename A>
	typename sbo_vector<T, C, A>::iterator sbo_vector<T, C, A>::end() noexcept
	{
		return data_ptr() + m_size;
	}

	template <typename T, sizet C, typename A>
	typename sbo_vector<T, C, A>::const_iterator sbo_vector<T, C, A>::end() const noexcept
	{
		return data_ptr() + m_size;
	}

	template <typename T, sizet C, typename A>
	typename sbo_vector<T, C, A>::const_iterator sbo_vector<T, C, A>::cend() const noexcept
	{
		return data_ptr() + m_size;
	}

	template <typename T, sizet C, typename A>
	bool sbo_vector<T, C, A>::empty() const noexcept { return m_size == 0; }

	template <typename T, sizet C, typename A>
	sizet sbo_vector<T, C, A>::size() const noexcept { return m_size; }

	template <typename T, sizet C, typename A>
	sizet sbo_vector<T, C, A>::capacity() const noexcept { return m_capacity; }

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::reserve(sizet new_capacity)
	{
		if (new_capacity > m_capacity)
		{
			grow_to_accommodate(new_capacity);
		}
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::clear() noexcept
	{
		destroy_elements();
		m_size = 0;
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::push_back(const T& value)
	{
		if (m_size >= m_capacity)
		{
			grow_to_accommodate(m_size + 1);
		}
		new(data_ptr() + m_size) T(value);
		m_size++;
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::push_back(T&& value)
	{
		if (m_size >= m_capacity)
		{
			grow_to_accommodate(m_size + 1);
		}
		new(data_ptr() + m_size) T(std::move(value));
		m_size++;
	}

	template <typename T, sizet C, typename A>
	template <typename... Args>
	T& sbo_vector<T, C, A>::emplace_back(Args&&... args)
	{
		if (m_size >= m_capacity)
		{
			grow_to_accommodate(m_size + 1);
		}
		T* ptr = data_ptr() + m_size;
		new(ptr) T(std::forward<Args>(args)...);
		m_size++;
		return *ptr;
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::pop_back() noexcept
	{
		if (m_size > 0)
		{
			m_size--;
			(data_ptr() + m_size)->~T();
		}
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::resize(sizet new_size)
	{
		if (new_size > m_size)
		{
			grow_to_accommodate(new_size);
			for (sizet i = m_size; i < new_size; ++i)
			{
				new(data_ptr() + i) T();
			}
		}
		else if (new_size < m_size)
		{
			for (sizet i = new_size; i < m_size; ++i)
			{
				(data_ptr() + i)->~T();
			}
		}
		m_size = new_size;
	}

	template <typename T, sizet C, typename A>
	void sbo_vector<T, C, A>::resize(sizet new_size, const T& value)
	{
		if (new_size > m_size)
		{
			grow_to_accommodate(new_size);
			for (sizet i = m_size; i < new_size; ++i)
			{
				new(data_ptr() + i) T(value);
			}
		}
		else if (new_size < m_size)
		{
			for (sizet i = new_size; i < m_size; ++i)
			{
				(data_ptr() + i)->~T();
			}
		}
		m_size = new_size;
	}

}
