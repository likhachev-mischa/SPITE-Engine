#pragma once
#include "base/CollectionAliases.hpp"
#include "base/VulkanUsage.hpp"

#include "engine/rendering/IResourceSet.hpp"
#include "engine/rendering/IResourceSetManager.hpp"
#include "engine/rendering/ResourcePool.hpp"

namespace spite
{
	class VulkanRenderDevice;

#if defined(SPITE_USE_DESCRIPTOR_SETS)
	// Represents a traditional Vulkan descriptor set.
	struct VulkanResourceSet : public IResourceSet
	{
		ResourceSetLayoutHandle layoutHandle;
		vk::DescriptorSet descriptorSet = nullptr;

		// These are not used in this mode, but are needed for the interface
		sizet getOffset() const override { return 0; }
		sizet getSize() const override { return 0; }
	};
#else
	// Represents an allocation within a descriptor buffer.
	struct VulkanResourceSet : public IResourceSet
	{
		ResourceSetLayoutHandle layoutHandle;
		BufferHandle buffer;
		vk::DeviceSize offset = 0;
		vk::DeviceSize size = 0;

		sizet getOffset() const override { return offset; }
		sizet getSize() const override { return size; }
	};
#endif

	class VulkanResourceSetManager : public IResourceSetManager
	{
	private:
		VulkanRenderDevice& m_device;
		HeapAllocator m_allocator;

#if defined(SPITE_USE_DESCRIPTOR_SETS)
		vk::DescriptorPool m_descriptorPool;
#else
		vk::PhysicalDeviceDescriptorBufferPropertiesEXT m_descriptorBufferProperties;

		// A buffer for resource descriptors (UBO, storage buffer, sampled image, etc.)
		BufferHandle m_resourceDescriptorBuffer;
		void* m_resourceDescriptorBufferPtr = nullptr;
		vk::DeviceSize m_resourceDescriptorBufferOffset = 0;

		// A separate buffer for sampler descriptors
		BufferHandle m_samplerDescriptorBuffer;
		void* m_samplerDescriptorBufferPtr = nullptr;
		vk::DeviceSize m_samplerDescriptorBufferOffset = 0;
#endif

		ResourcePool<VulkanResourceSet> m_sets;

	public:
		VulkanResourceSetManager(VulkanRenderDevice& device, const HeapAllocator& allocator);
		~VulkanResourceSetManager() override;

		VulkanResourceSetManager(const VulkanResourceSetManager&) = delete;
		VulkanResourceSetManager& operator=(const VulkanResourceSetManager&) = delete;
		VulkanResourceSetManager(VulkanResourceSetManager&&) = delete;
		VulkanResourceSetManager& operator=(VulkanResourceSetManager&&) = delete;

		ResourceSetHandle allocateSet(ResourceSetLayoutHandle layoutHandle) override;
		void updateSet(ResourceSetHandle setHandle, const sbo_vector<ResourceWrite>& writes) override;

#if !defined(SPITE_USE_DESCRIPTOR_SETS)
		BufferHandle getDescriptorBuffer(DescriptorType type) const override;
		sizet getDescriptorSize(DescriptorType type) const override;
#endif

		IResourceSet& getSet(ResourceSetHandle handle) override;
		const IResourceSet& getSet(ResourceSetHandle handle) const override;
	};
}
