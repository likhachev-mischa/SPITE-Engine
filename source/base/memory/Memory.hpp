#pragma once
#include <vcruntime_new.h>

#include "base/Platform.hpp"

//defines a global HeapAllocator instance that is used for non-specified allocations
//#define GLOBAL_ALLOCATOR 

void* operator new[](sizet size, const char* pName, int flags, unsigned debugFlags, const char* file, int line);

void* operator new[](sizet size, sizet alignment, sizet alignmentOffset, const char* pName, int flags,
                            unsigned debugFlags, const char* file, int line);

namespace spite
{
	inline constexpr sizet BYTE = 1;
	inline constexpr sizet KB = 1024 * BYTE;
	inline constexpr sizet MB = 1024 * KB;
	inline constexpr sizet GB = 1024 * MB;


	//CALL SHUTDOWN TO CHECK LEFTOVER ALLOCATIONS!
//	class BlockAllocator
//	{
//	public :
//		BlockAllocator(BlockAllocator&& other) = delete;
//		BlockAllocator& operator=(BlockAllocator&& other) = delete;
//
//		~BlockAllocator() = default;
//
//		BlockAllocator(const char* pName = EASTL_FIXED_POOL_DEFAULT_NAME);
//		BlockAllocator(const BlockAllocator& other);
//
//		BlockAllocator& operator=(const BlockAllocator& other);
//
//		void* allocate(sizet size, int flags = 0);
//		void* allocate(sizet size, sizet alignment, sizet offset, int flags = 0);
//		void deallocate(void* p, sizet n);
//
//		void init(void* pMemory, sizet memorySize, sizet nodeSize, sizet alignment,
//		          sizet alignmentOffset = 0);
//		void shutdown();
//
//	private:
//		eastl::fixed_allocator m_allocator;
//		sizet m_totalBytes = 0;
//	};
}

