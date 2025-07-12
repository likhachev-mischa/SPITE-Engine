#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

// Test Shared Components
struct Material : spite::ISharedComponent
{
	float r, g, b;
	Material() = default;


	Material(float r, float g, float b): r(r), g(g), b(b)
	{
	}

	bool operator==(const Material& other) const
	{
		constexpr float epsilon = std::numeric_limits<float>::
			epsilon();
		return (std::abs(r - other.r) < epsilon) &&
			(std::abs(g - other.g) < epsilon) &&
			(std::abs(b - other.b) < epsilon);
	}

	struct Hash
	{
		size_t operator()(const Material& m) const
		{
			size_t hash = std::hash<float>()(m.r);
			hash = hash_combine(hash, std::hash<float>()(m.g));
			hash = hash_combine(hash, std::hash<float>()(m.b));
			return hash;
		}

		template <class T>
		static size_t hash_combine(size_t seed, const T& v)
		{
			return seed ^ (std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
		}
	};

	    struct Equals { bool operator()(const Material& a, const Material& z) const { return a == z; } };
};

class EcsSharedComponentTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsSharedComponentTestAllocator", 32 * spite::MB)
		{
		}

		~Allocators() { allocator.shutdown(); }
	};

	struct Container
	{
		spite::ComponentMetadataRegistry metadataRegistry;
		spite::AspectRegistry aspectRegistry;
		spite::VersionManager versionManager;
		spite::ArchetypeManager archetypeManager;
		spite::SharedComponentManager sharedComponentManager;
		spite::SharedComponentRegistryBridge registryBridge;
		spite::EntityManager entityManager;
		spite::ComponentMetadataInitializer componentInitializer;
		spite::ScratchAllocator scratchAllocator;
		spite::QueryRegistry queryRegistry;

		Container(spite::HeapAllocator& allocator) :
			aspectRegistry(allocator)
			, versionManager(allocator, &aspectRegistry)
			, archetypeManager(&metadataRegistry, allocator, &aspectRegistry, &versionManager)
			, sharedComponentManager(metadataRegistry, allocator)
			, queryRegistry(allocator, &archetypeManager, &versionManager, &aspectRegistry, &metadataRegistry)
			, entityManager(archetypeManager, sharedComponentManager, metadataRegistry, &aspectRegistry, &queryRegistry)
			, registryBridge(&sharedComponentManager)
			, componentInitializer(&metadataRegistry, &registryBridge)
			, scratchAllocator(1 * spite::MB)
		{
			componentInitializer.registerComponent<spite::SharedComponent<Material>>();
		}
	};

	Allocators* allocContainer = new Allocators;
	Container* container = allocContainer->allocator.new_object<Container>(allocContainer->allocator);

	spite::HeapAllocator& allocator = allocContainer->allocator;
	spite::ComponentMetadataRegistry& metadataRegistry = container->metadataRegistry;
	spite::AspectRegistry& aspectRegistry = container->aspectRegistry;
	spite::VersionManager& versionManager = container->versionManager;
	spite::ArchetypeManager& archetypeManager = container->archetypeManager;
	spite::SharedComponentManager& sharedComponentManager = container->sharedComponentManager;
	spite::QueryRegistry& queryRegistry = container->queryRegistry;
	spite::EntityManager& entityManager = container->entityManager;
	spite::ScratchAllocator& scratchAllocator = container->scratchAllocator;
	spite::ComponentMetadataInitializer& componentInitializer = container->componentInitializer;


	EcsSharedComponentTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsSharedComponentTest, SetAndGetSharedComponent)
{
	auto e = entityManager.createEntity();
	entityManager.setShared<Material>(e,Material(1.0f, 0.0f, 0.0f));
	ASSERT_TRUE(entityManager.hasComponent<spite::SharedComponent<Material>>(e));
	const auto& mat = entityManager.getShared<Material>(e);
	ASSERT_EQ(mat.r, 1.0f);
}

TEST_F(EcsSharedComponentTest, MultipleEntitiesShareComponent)
{
	auto e1 = entityManager.createEntity();
	auto e2 = entityManager.createEntity();
	entityManager.setShared<Material>(e1, Material(0.5f, 0.5f, 0.5f));
	entityManager.setShared<Material>(e2, Material(0.5f, 0.5f, 0.5f));

	const auto& mat1 = entityManager.getShared<Material>(e1);
	const auto& mat2 = entityManager.getShared<Material>(e2);
	ASSERT_EQ(&mat1, &mat2); // Should point to the same data
}

TEST_F(EcsSharedComponentTest, ModifySharedComponent)
{
	auto e1 = entityManager.createEntity();
	auto e2 = entityManager.createEntity();
	entityManager.setShared<Material>(e1, Material(0.5f, 0.5f, 0.5f));
	entityManager.setShared<Material>(e2, Material(0.5f, 0.5f, 0.5f));

	// This should perform a copy-on-write
	auto& mat1_mut = entityManager.getMutableShared<Material>(e1);
	mat1_mut.r = 1.0f;

	const auto& mat1_after = entityManager.getShared<Material>(e1);
	const auto& mat2_after = entityManager.getShared<Material>(e2);

	ASSERT_NE(&mat1_after, &mat2_after);
	ASSERT_EQ(mat1_after.r, 1.0f);
	ASSERT_EQ(mat2_after.r, 0.5f);
}
