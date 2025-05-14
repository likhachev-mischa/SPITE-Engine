#pragma once
#include "base/VmaUsage.hpp"
#include "base/Platform.hpp"
#include "engine/Common.hpp"

namespace spite
{
	struct BufferWrapper
	{
		BufferWrapper(const BufferWrapper& other) = delete;
		BufferWrapper& operator=(const BufferWrapper& other) = delete;

		vk::Buffer buffer;
		vma::Allocation allocation;

		vma::Allocator allocator;
		vk::DeviceSize size{};

		BufferWrapper();

		BufferWrapper(const u64 size,
		              const vk::BufferUsageFlags& usage,
		              const vk::MemoryPropertyFlags memoryProperty,
		              const vma::AllocationCreateFlags& allocationFlag,
		              const QueueFamilyIndices& indices,
		              const vma::Allocator& allocator);

		BufferWrapper(BufferWrapper&& other) noexcept;

		BufferWrapper& operator=(BufferWrapper&& other) noexcept;

		//copies only same sized buffers
		void copyBuffer(const BufferWrapper& other,
		                const vk::Device& device,
		                const vk::CommandPool& transferCommandPool,
		                const vk::Queue transferQueue,
		                const vk::AllocationCallbacks* allocationCallbacks) const;

		void copyMemory(const void* data,
		                const vk::DeviceSize& memorySize,
		                const vk::DeviceSize& localOffset) const;

		[[nodiscard]] void* mapMemory() const;

		void unmapMemory() const;

		void destroy();

		~BufferWrapper();
	};
}
