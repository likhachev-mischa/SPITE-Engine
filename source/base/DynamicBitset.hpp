#pragma once

#include "base/CollectionAliases.hpp"
#include "base/Platform.hpp"

#include "CollectionUtilities.hpp"

namespace spite
{
	// A bitset that can be resized at runtime.
	class DynamicBitset
	{
	private:
		heap_vector<u64> m_blocks;
		sizet m_size = 0;

		static constexpr int BITS_PER_BLOCK = 64;

		static sizet block_index(sizet pos) { return pos / BITS_PER_BLOCK; }
		static sizet bit_index(sizet pos) { return pos % BITS_PER_BLOCK; }

	public:
		DynamicBitset(const HeapAllocator& allocator) : m_blocks(makeHeapVector<u64>(allocator))
		{
		}

		DynamicBitset(sizet num_bits, const HeapAllocator& allocator)
			: m_blocks(makeHeapVector<u64>(allocator))
		{
			resize(num_bits);
		}

		DynamicBitset(sizet num_bits, const HeapAllocatorAdapter<u64>& allocator)
			: m_blocks(allocator)
		{
			resize(num_bits);
		}

		void resize(sizet num_bits)
		{
			m_size = num_bits;
			const sizet num_blocks = (num_bits + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
			m_blocks.resize(num_blocks, 0);
		}

		DynamicBitset& set(sizet pos, bool value = true)
		{
			if (pos >= m_size)
			{
				resize(pos + 1);
			}
			if (value)
			{
				m_blocks[block_index(pos)] |= (1ULL << bit_index(pos));
			}
			else
			{
				m_blocks[block_index(pos)] &= ~(1ULL << bit_index(pos));
			}
			return *this;
		}

		bool test(sizet pos) const
		{
			if (pos >= m_size) return false;
			return (m_blocks[block_index(pos)] >> bit_index(pos)) & 1ULL;
		}

		bool any() const
		{
			for (u64 block : m_blocks)
			{
				if (block != 0) return true;
			}
			return false;
		}

		sizet size() const { return m_size; }

		DynamicBitset operator|(const DynamicBitset& other) const
		{
			DynamicBitset result(std::max(m_size, other.m_size), m_blocks.get_allocator());
			const sizet common_blocks = std::min(m_blocks.size(), other.m_blocks.size());
			for (sizet i = 0; i < common_blocks; ++i)
			{
				result.m_blocks[i] = m_blocks[i] | other.m_blocks[i];
			}
			// Copy remaining blocks from the larger bitset
			if (m_blocks.size() > other.m_blocks.size())
			{
				for (sizet i = common_blocks; i < m_blocks.size(); ++i)
				{
					result.m_blocks[i] = m_blocks[i];
				}
			}
			else
			{
				for (sizet i = common_blocks; i < other.m_blocks.size(); ++i)
				{
					result.m_blocks[i] = other.m_blocks[i];
				}
			}
			return result;
		}

		DynamicBitset operator&(const DynamicBitset& other) const
		{
			DynamicBitset result(std::max(m_size, other.m_size), m_blocks.get_allocator());
			const sizet num_blocks = std::min(m_blocks.size(), other.m_blocks.size());
			for (sizet i = 0; i < num_blocks; ++i)
			{
				result.m_blocks[i] = m_blocks[i] & other.m_blocks[i];
			}
			return result;
		}
	};
}
