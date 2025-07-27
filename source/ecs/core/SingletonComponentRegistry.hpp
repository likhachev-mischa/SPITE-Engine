#pragma once

#include <memory>
#include <utility>
#include <typeindex>
#include <mutex>
#include <functional>

#include "base/Assert.hpp"
#include "base/CollectionAliases.hpp"
#include "ecs/core/IComponent.hpp"

namespace spite
{
	class SingletonComponentRegistry
	{
	private:
		struct ISingletonWrapper
		{
			virtual ~ISingletonWrapper() = default;
		};

		template <typename T>
		struct SingletonWrapper : ISingletonWrapper
		{
			std::unique_ptr<T> instance;
			SingletonWrapper() : instance(std::make_unique<T>()) {}
		};

		heap_unordered_map<std::type_index, std::unique_ptr<ISingletonWrapper>, std::hash<std::type_index>> m_instances;
		heap_unordered_map<std::type_index, std::mutex*, std::hash<std::type_index>> m_mutexes;
		HeapAllocator m_allocator;

		std::mutex& getMutexForType(const std::type_index& typeIndex)
		{
			auto it = m_mutexes.find(typeIndex);
			if (it == m_mutexes.end())
			{
				it = m_mutexes.emplace(typeIndex, m_allocator.new_object<std::mutex>()).first;
			}
			return *it->second;
		}

		std::mutex& getMutexForType(const std::type_index& typeIndex) const
		{
			auto it = m_mutexes.find(typeIndex);
			SASSERTM(it != m_mutexes.end(), "Mutex for singleton of type %s was accessed before it was created\n", typeIndex.name())
			return *it->second;
		}

	public:
		SingletonComponentRegistry(const HeapAllocator& allocator);

		SingletonComponentRegistry(const SingletonComponentRegistry&) = delete;
		SingletonComponentRegistry& operator=(const SingletonComponentRegistry&) = delete;
		SingletonComponentRegistry(SingletonComponentRegistry&&) = delete;
		SingletonComponentRegistry& operator=(SingletonComponentRegistry&&) = delete;

		~SingletonComponentRegistry();

		template <t_singleton_component T>
		void registerSingleton(std::unique_ptr<T> instance = nullptr)
		{
			const auto typeIndex = std::type_index(typeid(T));
			auto wrapper = std::make_unique<SingletonWrapper<T>>();
			if(instance)
			{
				wrapper->instance = std::move(instance);
			}
			m_instances[typeIndex] = std::move(wrapper);
		}

		//not thread-safe
		template <t_singleton_component T>
		T& get()
		{
			const auto typeIndex = std::type_index(typeid(T));
			auto it = m_instances.find(typeIndex);
			if (it == m_instances.end())
			{
				registerSingleton<T>();
				it = m_instances.find(typeIndex);
			}
			auto* wrapper = static_cast<SingletonWrapper<T>*>(it->second.get());
			return *wrapper->instance;
		}

		//not thread-safe
		template <t_singleton_component T>
		const T& get() const
		{
			const auto typeIndex = std::type_index(typeid(T));
			auto it = m_instances.find(typeIndex);
			SASSERTM(it != m_instances.end(), "Singleton of type %s was accessed before it was created\n", typeid(T).name())
			const auto* wrapper = static_cast<const SingletonWrapper<T>*>(it->second.get());
			return *wrapper->instance;
		}

		//thread-safe
		template<t_singleton_component T>
		void access(std::function<void(T&)> accessor)
		{
			const auto typeIndex = std::type_index(typeid(T));
			std::lock_guard<std::mutex> lock(getMutexForType(typeIndex));

			auto it = m_instances.find(typeIndex);
			if (it == m_instances.end()) {
				registerSingleton<T>();
				it = m_instances.find(typeIndex);
			}
			auto* wrapper = static_cast<SingletonWrapper<T>*>(it->second.get());
			accessor(*wrapper->instance);
		}

		//thread-safe
		template<t_singleton_component T>
		void access(std::function<void(const T&)> accessor) const
		{
			const auto typeIndex = std::type_index(typeid(T));
			std::lock_guard<std::mutex> lock(getMutexForType(typeIndex));

			auto it = m_instances.find(typeIndex);
			SASSERTM(it != m_instances.end(), "Singleton of type %s was accessed before it was created.", typeid(T).name())
			const auto* wrapper = static_cast<const SingletonWrapper<T>*>(it->second.get());
			accessor(*wrapper->instance);
		}
	};
}
