#include "VulkanBuffer.hpp"

#include "engine/VulkanResources.hpp"

namespace spite
{
	BufferWrapper::BufferWrapper() = default;

	BufferWrapper::BufferWrapper(const u64 size,
	                             const vk::BufferUsageFlags& usage,
	                             const vk::MemoryPropertyFlags memoryProperty,
	                             const vma::AllocationCreateFlags& allocationFlag,
	                             const QueueFamilyIndices& indices,
	                             const vma::Allocator& allocator) : allocator(allocator), size(size)
	{
		spite::createBuffer(size,
		                    usage,
		                    memoryProperty,
		                    allocationFlag,
		                    indices,
		                    allocator,
		                    buffer,
		                    allocation);
	}

	BufferWrapper::BufferWrapper(BufferWrapper&& other) noexcept : buffer(other.buffer),
		allocation(other.allocation), allocator(other.allocator), size(other.size)
	{
		other.allocator = nullptr;
		other.buffer = nullptr;
		other.allocation = nullptr;
	}

	BufferWrapper& BufferWrapper::operator=(BufferWrapper&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		buffer = other.buffer;
		allocation = other.allocation;
		allocator = other.allocator;
		size = other.size;

		other.allocator = nullptr;
		other.buffer = nullptr;
		other.allocation = nullptr;

		return *this;
	}

	void BufferWrapper::copyBuffer(const BufferWrapper& other,
	                               const vk::Device& device,
	                               const vk::CommandPool& transferCommandPool,
	                               const vk::Queue transferQueue,
	                               const vk::AllocationCallbacks* allocationCallbacks) const
	{
		SASSERTM(size == other.size, "Buffer sizes are not equal")
		spite::copyBuffer(other.buffer,
		                  buffer,
		                  size,
		                  transferCommandPool,
		                  device,
		                  transferQueue,
		                  allocationCallbacks);
	}

	void BufferWrapper::copyMemory(const void* data,
	                               const vk::DeviceSize& memorySize,
	                               const vk::DeviceSize& localOffset) const
	{
		vk::Result result = allocator.copyMemoryToAllocation(
			data,
			allocation,
			localOffset,
			memorySize);
		SASSERT_VULKAN(result)
	}

	void* BufferWrapper::mapMemory() const
	{
		auto [result, memory] = allocator.mapMemory(allocation);
		SASSERT_VULKAN(result)
		return memory;
	}

	void BufferWrapper::unmapMemory() const
	{
		allocator.unmapMemory(allocation);
	}

	void BufferWrapper::destroy()
	{
		allocator.destroyBuffer(buffer, allocation);
		allocator = nullptr;
	}

	BufferWrapper::~BufferWrapper()
	{
		if (allocator) allocator.destroyBuffer(buffer, allocation);
	}
}
