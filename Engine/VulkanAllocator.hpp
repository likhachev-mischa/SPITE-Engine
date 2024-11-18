#pragma once
#include "VulkanUsage.hpp"
#include "Base/Logging.hpp"
#include "Base/Platform.hpp"
#include "Base/Memory.hpp"

namespace spite
{
	//uses heap allocator
	//allocation scope is unused for now
	void* vkAllocate(void* pUserData, sizet size, sizet alignment,
	                 VkSystemAllocationScope systemAllocationScope)
	{
		auto allocator = static_cast<spite::HeapAllocator*>(pUserData);
		return allocator->allocate(size, alignment);
	}

	void* vkReallocate(void* pUserData, void* pOriginal, sizet size, sizet alignment,
	                   VkSystemAllocationScope systemAllocationScope)
	{
		auto allocator = static_cast<spite::HeapAllocator*>(pUserData);
		return allocator->reallocate(pOriginal, size);
	}

	void vkFree(void* pUserData, void* pMemory)
	{
		auto allocator = static_cast<spite::HeapAllocator*>(pUserData);
		allocator->deallocate(pMemory);
	}

	void vkAllocationCallback(void* pUserData, sizet size, VkInternalAllocationType allocationType,
	                          VkSystemAllocationScope allocationScope)
	{
		auto allocator = static_cast<spite::HeapAllocator*>(pUserData);
		SDEBUG_LOG("Allocator %s allocated %llu mem for Vulkan", allocator->get_name(), size)
	}

	void vkFreeCallback(void* pUserData, sizet size, VkInternalAllocationType allocationType,
	                    VkSystemAllocationScope allocationScope)
	{
		auto allocator = static_cast<spite::HeapAllocator*>(pUserData);
		SDEBUG_LOG("Allocator %s freed %llu mem for Vulkan", allocator->get_name(), size)
	}
}
