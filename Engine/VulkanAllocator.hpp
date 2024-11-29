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
	                 VkSystemAllocationScope systemAllocationScope);

	void* vkReallocate(void* pUserData, void* pOriginal, sizet size, sizet alignment,
	                   VkSystemAllocationScope systemAllocationScope);

	void vkFree(void* pUserData, void* pMemory);

	void vkAllocationCallback(void* pUserData, sizet size, VkInternalAllocationType allocationType,
	                          VkSystemAllocationScope allocationScope);

	void vkFreeCallback(void* pUserData, sizet size, VkInternalAllocationType allocationType,
	                    VkSystemAllocationScope allocationScope);
}
