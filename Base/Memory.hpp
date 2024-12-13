#pragma once
#include <EASTL/allocator.h>
#include <EASTL/fixed_allocator.h>

#include "Platform.hpp"

namespace spite
{
	constexpr sizet MAXIMUM_DINAMIC_SIZE = 32 * 1024 * 1024;

	struct MemoryStatistics
	{
		sizet allocatedBytes;
		sizet totalBytes;

		u32 allocationCount;

		void add(sizet value);
	};

	//CALL SHUTDOWN TO DISPOSE!
	class HeapAllocator
	{
	public:
		HeapAllocator(HeapAllocator&& other) = delete;
		HeapAllocator& operator=(HeapAllocator&& other) = delete;
		HeapAllocator& operator=(const HeapAllocator& x);

		~HeapAllocator() = default;

		HeapAllocator(const char* pName = EASTL_NAME_VAL(EASTL_ALLOCATOR_DEFAULT_NAME),
		              sizet size = MAXIMUM_DINAMIC_SIZE);
		HeapAllocator(const HeapAllocator& x);
		HeapAllocator(const HeapAllocator& x, const char* pName);


		void* allocate(size_t size, int flags = 0);
		void* allocate(size_t size, size_t alignment, size_t offset = 0, int flags = 0);
		void* reallocate(void* original, sizet size);
		void deallocate(void* p, size_t n = 0);

		const char* get_name() const;
		void set_name(const char* pName);

		void shutdown();

	protected:
		bool operator==(const HeapAllocator& b);
		bool operator!=(const HeapAllocator& b);

	private:
		void init(sizet size);
		cstring m_name;

		void* m_tlsfHandle{};
		void* m_memory{};
		sizet m_maxSize;
	};

	//CALL SHUTDOWN TO CHECK LEFTOVER ALLOCATIONS!
	class BlockAllocator
	{
	public :
		BlockAllocator(BlockAllocator&& other) = delete;
		BlockAllocator& operator=(BlockAllocator&& other) = delete;

		~BlockAllocator() = default;

		BlockAllocator(const char* pName = EASTL_FIXED_POOL_DEFAULT_NAME);
		BlockAllocator(const BlockAllocator& other);

		BlockAllocator& operator=(const BlockAllocator& other);

		void* allocate(size_t size, int flags = 0);
		void* allocate(size_t size, size_t alignment, size_t offset, int flags = 0);
		void deallocate(void* p, size_t n);

		void init(void* pMemory, sizet memorySize, sizet nodeSize, sizet alignment,
		          sizet alignmentOffset = 0);
		void shutdown();

	private:
		eastl::fixed_allocator m_allocator;
		sizet m_totalBytes = 0;
	};

}
