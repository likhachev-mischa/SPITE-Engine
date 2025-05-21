#include "CoreSystems.hpp"

#include "engine/VulkanResources.hpp"

namespace spite
{
	void TextureLoadSystem::onInitialize()
	{
		requireComponent(typeid(TextureLoadRequest));
	}

	void TextureLoadSystem::onUpdate(float deltaTime)
	{
		auto& requestTable = m_entityService->componentStorage()->getEventsAsserted<
			TextureLoadRequest>();


		auto componentManager = m_entityService->componentManager();
		const auto& deviceComponent = componentManager->getSingleton<DeviceComponent>();
		const QueueFamilyIndices& queueFamilyIndices = deviceComponent.queueFamilyIndices;
		const vk::Device device = deviceComponent.device;
		auto& queueComponent = componentManager->getSingleton<QueueComponent>();

		const vma::Allocator& gpuAllocator = componentManager->getSingleton<GpuAllocatorComponent>()
		                                                     .allocator;

		const auto& allocationCallbacksComponent = componentManager->getSingleton<
			AllocationCallbacksComponent>();
		const auto& commandPoolComponent = componentManager->getSingleton<CommandPoolComponent>();

		Entity textureLayoutEntity = m_entityService->entityManager()->getNamedEntity(
			"TextureDescriptorLayout");
		const auto& textureDescriptorLayoutComponent = componentManager->getComponent<
			DescriptorSetLayoutComponent>(textureLayoutEntity);
		for (const auto& request : requestTable)
		{
			sizet targetsSize = request.targets.size();
			SASSERTM(targetsSize != 0,
			         "Invalid entities count upon texture %s load!\n",
			         request.path.c_str())

			Image texture = createTexture(request.path.c_str(),
			                              queueFamilyIndices,
			                              gpuAllocator,
			                              device,
			                              queueComponent.transferQueue,
			                              commandPoolComponent.transferCommandPool,
			                              queueComponent.graphicsQueue,
			                              commandPoolComponent.graphicsCommandPool);

			vk::ImageView textureImageView = createImageView(
				device,
				texture,
				vk::ImageAspectFlagBits::eColor,
				allocationCallbacksComponent.allocationCallbacks);

			vk::Sampler textureSampler = createSampler(device,
			                                           allocationCallbacksComponent.
			                                           allocationCallbacks);

			auto descriptorPool = createDescriptorPool(device,
			                                           &allocationCallbacksComponent.
			                                           allocationCallbacks,
			                                           vk::DescriptorType::eCombinedImageSampler,
			                                           MAX_FRAMES_IN_FLIGHT);
			auto descriptorSets = createDescriptorSets(device,
			                                           textureDescriptorLayoutComponent.layout,
			                                           descriptorPool,
			                                           MAX_FRAMES_IN_FLIGHT);
			DescriptorPoolComponent texturePool;
			texturePool.maxSets = MAX_FRAMES_IN_FLIGHT;
			texturePool.pool = descriptorPool;
			DescriptorSetsComponent textureSets;
			std::copy_n(descriptorSets.begin(),
			            MAX_FRAMES_IN_FLIGHT,
			            textureSets.descriptorSets.begin());
			Entity textureDescriptorEntity = m_entityService->entityManager()->createEntity();
			componentManager->addComponent(textureDescriptorEntity, texturePool);
			componentManager->addComponent(textureDescriptorEntity, textureSets);

			TextureComponent textureComponent;
			textureComponent.descriptorEntity = textureDescriptorEntity;
			textureComponent.texture = texture;
			textureComponent.imageView = textureImageView;
			textureComponent.sampler = textureSampler;

			vk::DescriptorImageInfo imageInfo(textureComponent.sampler,
			                                  textureComponent.imageView,
			                                  vk::ImageLayout::eShaderReadOnlyOptimal);

			std::vector<vk::WriteDescriptorSet> descriptorWrites;
			descriptorWrites.reserve(descriptorSets.size());
			for (const auto& textureDescriptor : descriptorSets)
			{
				descriptorWrites.emplace_back(textureDescriptor,
				                              0,
				                              0,
				                              1,
				                              vk::DescriptorType::eCombinedImageSampler,
				                              &imageInfo);
			}

			device.updateDescriptorSets(static_cast<u32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

			Entity target = request.targets[0];
			componentManager->addComponent(target, textureComponent);

			for (sizet i = 1; i < targetsSize; ++i)
			{
				componentManager->addComponent<TextureComponent>(target, request.targets[i]);
			}

			SDEBUG_LOG("Texture %s succesfully loaded!\n", request.path.c_str())
		}
		m_entityService->entityEventManager()->rewindEvent(typeid(TextureLoadRequest));
	}
}
