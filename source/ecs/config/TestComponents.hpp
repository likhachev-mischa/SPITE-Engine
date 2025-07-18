#pragma once
//all test components should be here
#ifdef SPITE_TEST
#include <tuple>
#include <limits> 
#include <cmath>  

#include "ecs/core/IComponent.hpp"


namespace spite
{
	namespace test
	{
		struct Position : spite::IComponent
		{
			float x, y, z;
			Position() = default;

			Position(float x, float y, float z) : x(x), y(y), z(z)
			{
			}
		};

		struct Velocity : spite::IComponent
		{
			float dx, dy, dz;

			Velocity() = default;

			Velocity(float x, float y, float z) : dx(x), dy(y), dz(z)
			{
			}
		};

		struct Tag : spite::IComponent
		{
		};

		struct Transform : spite::IComponent
		{
			float x, y, z;
		};

		struct Renderable : spite::IComponent
		{
			int meshId;
		};
		struct LifecycleComponent : spite::IComponent
		{
			int* constructor_calls_ptr = nullptr;
			int* destructor_calls_ptr = nullptr;
			int* move_constructor_calls_ptr = nullptr;
			int* data = nullptr;

			LifecycleComponent() : data(new int(10))
			{
			}

			LifecycleComponent(int* ctor_ptr, int* dtor_ptr, int* move_ctor_ptr)
				: constructor_calls_ptr(ctor_ptr)
				, destructor_calls_ptr(dtor_ptr)
				, move_constructor_calls_ptr(move_ctor_ptr)
				, data(new int(10))
			{
				if (constructor_calls_ptr)
				{
					(*constructor_calls_ptr)++;
				}
			}

			~LifecycleComponent()
			{
				if (destructor_calls_ptr)
				{
					(*destructor_calls_ptr)++;
				}
				delete data;
			}

			LifecycleComponent(const LifecycleComponent&) = delete;
			LifecycleComponent& operator=(const LifecycleComponent&) = delete;

			LifecycleComponent(LifecycleComponent&& other) noexcept
				: constructor_calls_ptr(other.constructor_calls_ptr)
				, destructor_calls_ptr(other.destructor_calls_ptr)
				, move_constructor_calls_ptr(other.move_constructor_calls_ptr)
				, data(other.data)
			{
				if (move_constructor_calls_ptr)
				{
					(*move_constructor_calls_ptr)++;
				}
				other.data = nullptr; 
			}

			LifecycleComponent& operator=(LifecycleComponent&& other) noexcept
			{
				if (this != &other)
				{
					delete data;
					constructor_calls_ptr = other.constructor_calls_ptr;
					destructor_calls_ptr = other.destructor_calls_ptr;
					move_constructor_calls_ptr = other.move_constructor_calls_ptr;
					data = other.data;
					other.data = nullptr;
				}
				return *this;
			}
		};

		struct OtherComponent : spite::IComponent
		{
		};

		struct Rotator : spite::IComponent
		{
			float pitch, yaw, roll;
		};

		struct ComponentA : spite::IComponent
		{
		};

		struct ComponentB : spite::IComponent
		{
		};

		struct TestSingletonA : spite::ISingletonComponent
		{
			int value = 10;
		};

		struct TestSingletonB : spite::ISingletonComponent
		{
			float value = 20.0f;
		};

		struct TagA : spite::IComponent
		{
		};

		struct TagB : spite::IComponent
		{
		};


		struct Material : spite::ISharedComponent
		{
			float r, g, b;
			Material() = default;


			Material(float r, float g, float b) : r(r), g(g), b(b)
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

			struct Equals
			{
				bool operator()(const Material& a, const Material& z) const { return a == z; }
			};
		};
	}

	using ComponentList = std::tuple<test::Position, test::Velocity,
	                                 test::Tag, test::Transform, test::Renderable, test::OtherComponent, test::Rotator,
	                                 test::ComponentA, test::ComponentB, test::TagA, test::TagB,
	                                 test::LifecycleComponent, SharedComponent<test::Material>>;
	using SingletonComponentList = std::tuple<test::TestSingletonA, test::TestSingletonB>;
}

#endif
