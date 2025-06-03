#pragma once
#include <bitset>

#include "HeapAllocator.hpp"

namespace spite
{
    // Pool-based allocator for same-sized objects
    template<typename T, size_t BlockSize = 1024>
    class PoolAllocator
    {
    private:
        struct Block
        {
            alignas(T) char data[sizeof(T) * BlockSize];
            std::bitset<BlockSize> used;
            Block* next = nullptr;
        };

        Block* m_currentBlock = nullptr;
        static inline PoolAllocator* m_instance = nullptr;

        PoolAllocator();

        Block* allocate_new_block();

    public:
        static PoolAllocator& instance();

        T* allocate();

        void deallocate(T* ptr);

        void cleanup();

        ~PoolAllocator();
    };

    template <typename T, size_t BlockSize>
    PoolAllocator<T, BlockSize>::PoolAllocator() = default;

    template <typename T, size_t BlockSize>
    typename PoolAllocator<T, BlockSize>::Block* PoolAllocator<T, BlockSize>::allocate_new_block()
    {
        Block* new_block = static_cast<Block*>(getGlobalAllocator().allocate(sizeof(Block), alignof(Block)));
        new (new_block) Block();
        new_block->next = m_currentBlock;
        m_currentBlock = new_block;
        return new_block;
    }

    template <typename T, size_t BlockSize>
    PoolAllocator<T, BlockSize>& PoolAllocator<T, BlockSize>::instance()
    {
        if (!m_instance) {
            m_instance = static_cast<PoolAllocator*>(getGlobalAllocator().allocate(sizeof(PoolAllocator), alignof(PoolAllocator)));
            new (m_instance) PoolAllocator();
        }
        return *m_instance;
    }

    template <typename T, size_t BlockSize>
    T* PoolAllocator<T, BlockSize>::allocate()
    {
        Block* block = m_currentBlock;
        while (block) {
            for (size_t i = 0; i < BlockSize; ++i) {
                if (!block->used[i]) {
                    block->used[i] = true;
                    return reinterpret_cast<T*>(block->data + i * sizeof(T));
                }
            }
            block = block->next;
        }

        // No free slot found, allocate new block
        block = allocate_new_block();
        block->used[0] = true;
        return reinterpret_cast<T*>(block->data);
    }

    template <typename T, size_t BlockSize>
    void PoolAllocator<T, BlockSize>::deallocate(T* ptr)
    {
        Block* block = m_currentBlock;
        while (block) {
            char* block_start = block->data;
            char* block_end = block->data + sizeof(T) * BlockSize;
            char* ptr_char = reinterpret_cast<char*>(ptr);

            if (ptr_char >= block_start && ptr_char < block_end) {
                size_t index = (ptr_char - block_start) / sizeof(T);
                block->used[index] = false;
                return;
            }
            block = block->next;
        }
    }

    template <typename T, size_t BlockSize>
    void PoolAllocator<T, BlockSize>::cleanup()
    {
        while (m_currentBlock) {
            Block* next = m_currentBlock->next;
            m_currentBlock->~Block();
            getGlobalAllocator().deallocate(m_currentBlock);
            m_currentBlock = next;
        }
    }

    template <typename T, size_t BlockSize>
    PoolAllocator<T, BlockSize>::~PoolAllocator()
    {
        cleanup();
    }
}
