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

			SingletonWrapper() : instance(std::make_unique<T>())
			{
			}
		};

		heap_unordered_map<std::type_index, std::unique_ptr<ISingletonWrapper>, std::hash<std::type_index>> m_instances;
		heap_unordered_map<std::type_index, std::unique_ptr<std::mutex>, std::hash<std::type_index>> m_mutexes;
		mutable std::mutex m_registryMutex;

	public:
		SingletonComponentRegistry(const HeapAllocator& allocator);

		SingletonComponentRegistry(const SingletonComponentRegistry&) = delete;
		SingletonComponentRegistry& operator=(const SingletonComponentRegistry&) = delete;
		SingletonComponentRegistry(SingletonComponentRegistry&&) = delete;
		SingletonComponentRegistry& operator=(SingletonComponentRegistry&&) = delete;

		~SingletonComponentRegistry() = default;

		template <t_singleton_component T>
		void registerSingleton(std::unique_ptr<T> instance = nullptr)
		{
			const auto typeIndex = std::type_index(typeid(T));
			std::lock_guard<std::mutex> lock(m_registryMutex);

			auto wrapper = std::make_unique<SingletonWrapper<T>>();
			if (instance)
			{
				wrapper->instance = std::move(instance);
			}
			m_instances[typeIndex] = std::move(wrapper);

			if (m_mutexes.find(typeIndex) == m_mutexes.end())
			{
				m_mutexes.emplace(typeIndex, std::make_unique<std::mutex>());
			}
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
			SASSERTM(it != m_instances.end(), "Singleton of type %s was accessed before it was created\n",
			         typeid(T).name())
			const auto* wrapper = static_cast<const SingletonWrapper<T>*>(it->second.get());
			return *wrapper->instance;
		}

		//thread-safe
		template <t_singleton_component T>
		void access(std::function<void(T&)> accessor)
		{
			const auto typeIndex = std::type_index(typeid(T));

			std::unique_lock<std::mutex> registryLock(m_registryMutex);

			auto it = m_instances.find(typeIndex);
			if (it == m_instances.end())
			{
				auto wrapper = std::make_unique<SingletonWrapper<T>>();
				it = m_instances.emplace(typeIndex, std::move(wrapper)).first;
				m_mutexes.emplace(typeIndex, std::make_unique<std::mutex>());
			}

			auto& instanceMutex = *m_mutexes.at(typeIndex);
			auto* wrapperPtr = static_cast<SingletonWrapper<T>*>(it->second.get());

			registryLock.unlock();

			std::lock_guard<std::mutex> instanceLock(instanceMutex);
			accessor(*wrapperPtr->instance);
		}

		//thread-safe
		template <t_singleton_component T>
		void access(std::function<void(const T&)> accessor) const
		{
			const auto typeIndex = std::type_index(typeid(T));

			std::unique_lock<std::mutex> registryLock(m_registryMutex);

			auto it = m_instances.find(typeIndex);
			SASSERTM(it != m_instances.end(), "Singleton of type %s was accessed before it was created.",
			         typeid(T).name());

			auto& instanceMutex = *m_mutexes.at(typeIndex);
			const auto* wrapper = static_cast<const SingletonWrapper<T>*>(it->second.get());

			registryLock.unlock();

			std::lock_guard<std::mutex> instanceLock(instanceMutex);
			accessor(*wrapper->instance);
		}
	};
}
