#pragma once
#include <EASTL/allocator.h>
#include <EASTL/fixed_allocator.h>

#include "Platform.hpp"

//defines a global HeapAllocator instance that is used for non-specified allocations
#define GLOBAL_ALLOCATOR 

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

		//copied allocator manages the same memory pool
		//create new allocator if otherwise desired
		HeapAllocator(const HeapAllocator& x);
		HeapAllocator(const HeapAllocator& x, const char* pName);


		void* allocate(sizet size, int flags = 0);
		void* allocate(sizet size, sizet alignment, sizet offset = 0, int flags = 0);
		void* reallocate(void* original, sizet size);
		void deallocate(void* p, sizet n = 0);

		const char* get_name() const;
		void set_name(const char* pName);


		/**
		 * \brief 
		 * \param forceDealloc if true, does not check if any allocations remain and silently deallocates anything 
		 */
		void shutdown(bool forceDealloc = false);

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

	inline spite::HeapAllocator& getGlobalAllocator()
	{
		static spite::HeapAllocator instance("GlobalAllocator");
		return instance;
	}

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

		void* allocate(sizet size, int flags = 0);
		void* allocate(sizet size, sizet alignment, sizet offset, int flags = 0);
		void deallocate(void* p, sizet n);

		void init(void* pMemory, sizet memorySize, sizet nodeSize, sizet alignment,
		          sizet alignmentOffset = 0);
		void shutdown();

	private:
		eastl::fixed_allocator m_allocator;
		sizet m_totalBytes = 0;
	};
}

inline void* operator new[](sizet size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
#if defined (GLOBAL_ALLOCATOR)
	return spite::getGlobalAllocator().allocate(size);
#else
	return operator new[](size);
#endif
}

inline void* operator new[](sizet size, sizet alignment, sizet alignmentOffset, const char* pName, int flags,
                            unsigned debugFlags, const char* file, int line)
{
#if defined (GLOBAL_ALLOCATOR)
	return spite::getGlobalAllocator().allocate(size, alignment, alignmentOffset);
#else
	return operator new[](size, static_cast<std::align_val_t>(alignment));
#endif
}
