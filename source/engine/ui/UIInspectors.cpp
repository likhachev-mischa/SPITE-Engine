#include <array>
#include <format>
#include <functional>

#include <external/imgui/imgui.h>

#include "Reflection.hpp"
#include "TypeInspectorRegistry.hpp"
#include "ecs/core/EntityManager.hpp"
#include "ecs/query/Query.hpp"
#include "ecs/query/QueryBuilder.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/EngineComponents.hpp"

namespace spite
{
	void inspectUIEntityHierchy(EntityManager& entityManager)
	{
		ImGui::Begin("Entity Inspector");

		Entity selectedEntity;
		entityManager.accesSingleton<InspectedEntitySingleton>(
			[&selectedEntity](const InspectedEntitySingleton& singleton)
			{
				selectedEntity = singleton.entity;
			});

		std::function<void(Entity)> drawEntityNode =
			[&](Entity entity)
		{
			TransformRelationsComponent* relations = entityManager.tryGetComponent<TransformRelationsComponent>(entity);

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (selectedEntity == entity)
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			if (!relations|| relations->children.empty())
			{
				flags |= ImGuiTreeNodeFlags_Leaf;
			}

			std::array<char, 128> label_buffer;
			auto format_result = std::format_to_n(label_buffer.data(), label_buffer.size() - 1, "Entity {} gen {}",
			                                      entity.id(), entity.generation());
			*format_result.out = '\0';

			const bool node_open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entity.id())), flags,
			                                         "%s", label_buffer.data());

			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			{
				selectedEntity = entity;
			}

			if (node_open)
			{
				if (relations)
				{
					for (const Entity child : relations->children)
					{
						drawEntityNode(child);
					}
				}
				ImGui::TreePop();
			}
		};

		static QueryHandle query = entityManager.getQueryBuilder().with<Read<TransformComponent>>().build();

		for (const Entity entity : query.view<Entity>())
		{
			TransformRelationsComponent* relations= entityManager.tryGetComponent<TransformRelationsComponent>(entity);
			if (!relations|| relations->parent == Entity::undefined())
			{
				drawEntityNode(entity);
			}
		}

		ImGui::End();

		entityManager.accesSingleton<InspectedEntitySingleton>([&selectedEntity](InspectedEntitySingleton& singleton)
		{
			singleton.entity = selectedEntity;
		});
	}

	void inspectUIEntityComponents(EntityManager& entityManager)
	{
		Entity entity;
		entityManager.accesSingleton<InspectedEntitySingleton>([&entity](const InspectedEntitySingleton& singleton)
		{
			entity = singleton.entity;
		});

		if (!entityManager.isValid(entity))
		{
			ImGui::Text("Invalid Entity");
			return;
		}
		ImGui::Begin("Entity Components");

		ImGui::Text("Entity ID: %u, Gen: %u", entity.index(), entity.generation());

		const auto& archetype = entityManager.getArchetypeManager()->getEntityArchetype(entity);
		const auto& aspect = archetype.aspect();
		auto [chunk, indexInChunk] = archetype.getEntityLocation(entity);

		for (const auto& componentId : aspect.getComponentIds())
		{
			const auto* reflection = ReflectionRegistry::get()->getReflection(componentId);
			if (!reflection)
			{
				// Optional: Display components that are not reflected
				ImGui::TextDisabled("Unreflected Component: ID %u", componentId);
				continue;
			}

			if (ImGui::TreeNodeEx(reflection->name, ImGuiTreeNodeFlags_DefaultOpen))
			{
				void* componentData = chunk->getComponentDataPtrByIndex(
					archetype.getComponentIndex(componentId),
					indexInChunk
				);

				for (const auto& member : reflection->members)
				{
					void* memberData = static_cast<char*>(componentData) + member.offset;
					if (!TypeInspectorRegistry::get()->inspect(member.type, member.name.data(), memberData))
					{
						//ImGui::Text("%s: ", member.name.data());
						//ImGui::SameLine();
						//ImGui::TextDisabled("[Unsupported Type]");
					}
				}
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}
}
