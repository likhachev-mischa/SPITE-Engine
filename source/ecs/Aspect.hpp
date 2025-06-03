#pragma once
#include <typeindex>

#include <EASTL/vector.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include "base/Collections.hpp"

namespace spite
{	
	// Memory-optimized Aspect struct using Small Buffer Optimization
	// Most aspects contain only a few component types, so this avoids heap allocations
	// for small collections while eliminating HeapAllocator storage overhead
	/*
	struct Aspect
	{
	private:
		// Small Buffer Optimized vector - stores up to 8 types inline (zero allocations for most cases)
		// Falls back to global allocator for larger collections (no per-container allocator overhead)
		sbo_vector<std::type_index, 8> types;

	public:		// Default constructor - creates empty aspect
		Aspect() = default;

		// Constructor from initializer list
		Aspect(std::initializer_list<std::type_index> typeList)
			: types(typeList) {
			// Sort and remove duplicates
			eastl::sort(types.begin(), types.end());
			// Manual unique removal since sbo_vector doesn't have erase
			auto write_it = types.begin();
			for (auto read_it = types.begin(); read_it != types.end(); ++read_it) {
				if (write_it == types.begin() || *read_it != *(write_it - 1)) {
					*write_it = *read_it;
					++write_it;
				}
			}
			// Remove extra elements by adjusting size manually
			while (types.end() != write_it) {
				types.pop_back();
			}
		}
		// Constructor from iterator range
		template<typename Iterator>
		Aspect(Iterator begin, Iterator end) {
			types.reserve(eastl::distance(begin, end));
			for (auto it = begin; it != end; ++it) {
				types.push_back(*it);
			}
			eastl::sort(types.begin(), types.end());
			// Manual unique removal
			auto write_it = types.begin();
			for (auto read_it = types.begin(); read_it != types.end(); ++read_it) {
				if (write_it == types.begin() || *read_it != *(write_it - 1)) {
					*write_it = *read_it;
					++write_it;
				}
			}
			while (types.end() != write_it) {
				types.pop_back();
			}
		}

		// Constructor from single type
		explicit Aspect(std::type_index type) {
			types.push_back(type);
		}
		// Copy constructor
		Aspect(const Aspect& other) = default;

		// Move constructor
		Aspect(Aspect&& other) noexcept = default;

		// Copy assignment
		Aspect& operator=(const Aspect& other) = default;

		// Move assignment
		Aspect& operator=(Aspect&& other) noexcept = default;

		// Equality comparison
		bool operator==(const Aspect& other) const {
			return types == other.types;
		}

		// Inequality comparison
		bool operator!=(const Aspect& other) const {
			return !(*this == other);
		}

		// Less than comparison (for use in ordered containers)
		bool operator<(const Aspect& other) const {
			return types < other.types;
		}

		// Greater than comparison
		bool operator>(const Aspect& other) const {
			return other < *this;
		}

		// Less than or equal comparison
		bool operator<=(const Aspect& other) const {
			return !(other < *this);
		}

		// Greater than or equal comparison
		bool operator>=(const Aspect& other) const {
			return !(*this < other);
		}
		// Check if this aspect contains all types from another aspect (subset check)
		bool contains(const Aspect& other) const {
			// Manual implementation of subset check for sorted vectors
			auto it1 = types.begin();
			auto it2 = other.types.begin();
			
			while (it1 != types.end() && it2 != other.types.end()) {
				if (*it1 < *it2) {
					++it1;
				} else if (*it1 == *it2) {
					++it1;
					++it2;
				} else {
					// *it1 > *it2, meaning other has a type that this doesn't have
					return false;
				}
			}
			
			// If we've processed all of other's types, then this contains other
			return it2 == other.types.end();
		}

		// Check if this aspect contains a specific type
		bool contains(std::type_index type) const {
			return eastl::binary_search(types.begin(), types.end(), type);
		}

		// Check if this aspect intersects with another (has common types)
		bool intersects(const Aspect& other) const {
			// Use two-pointer technique for sorted vectors
			auto it1 = types.begin();
			auto it2 = other.types.begin();
			
			while (it1 != types.end() && it2 != other.types.end()) {
				if (*it1 == *it2) {
					return true;
				} else if (*it1 < *it2) {
					++it1;
				} else {
					++it2;
				}
			}
			return false;
		}

		// Get the number of types in this aspect
		size_t size() const {
			return types.size();
		}

		// Check if aspect is empty
		bool empty() const {
			return types.empty();
		}

		// Hash function for use in hash containers
		struct hash {
			size_t operator()(const Aspect& aspect) const {
				size_t seed = 0;
				for (const auto& type : aspect.types) {
					// Combine hashes using a simple hash combination method
					seed ^= std::hash<std::type_index>{}(type) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
				return seed;
			}
		};

		~Aspect() = default;
	};
	*/

	//class AspectRegistry
	//{
	//private:
	//	// Memory-efficient registry using global allocator with zero per-container overhead
	//	global_vector<Aspect> registeredAspects;
	//	
	//	// Optional: Use small buffer optimization for commonly accessed aspects
	//	SboVector<Aspect, 16> commonAspects;
	//	
	//	// Fast lookup cache for frequently used aspect combinations
	//	struct AspectCache {
	//		SboVector<std::type_index, 8> types;
	//		size_t aspectIndex;
	//		
	//		bool matches(const Aspect& aspect) const {
	//			if (types.size() != aspect.size()) return false;
	//			for (size_t i = 0; i < types.size(); ++i) {
	//				if (types[i] != aspect.types[i]) return false;
	//			}
	//			return true;
	//		}
	//	};
	//	
	//	SboVector<AspectCache, 8> recentlyUsed;

	//public:
	//	AspectRegistry() = default;
	//	
	//	// Register an aspect and return its index
	//	size_t registerAspect(const Aspect& aspect) {
	//		// Check if already registered
	//		for (size_t i = 0; i < registeredAspects.size(); ++i) {
	//			if (registeredAspects[i] == aspect) {
	//				return i;
	//			}
	//		}
	//		
	//		// Register new aspect
	//		size_t index = registeredAspects.size();
	//		registeredAspects.push_back(aspect);
	//		
	//		// Add to common aspects cache if it's small
	//		if (aspect.size() <= 4 && commonAspects.size() < 16) {
	//			commonAspects.pushBack(aspect);
	//		}
	//		
	//		return index;
	//	}
	//	
	//	// Get aspect by index
	//	const Aspect& getAspect(size_t index) const {
	//		return registeredAspects[index];
	//	}
	//	
	//	// Find aspect index with caching for performance
	//	size_t findAspectIndex(const Aspect& aspect) {
	//		// Check cache first
	//		for (const auto& cached : recentlyUsed) {
	//			if (cached.matches(aspect)) {
	//				return cached.aspectIndex;
	//			}
	//		}
	//		
	//		// Search in registered aspects
	//		for (size_t i = 0; i < registeredAspects.size(); ++i) {
	//			if (registeredAspects[i] == aspect) {
	//				// Add to cache
	//				if (recentlyUsed.size() < 8) {
	//					AspectCache cache;
	//					for (const auto& type : aspect.types) {
	//						cache.types.pushBack(type);
	//					}
	//					cache.aspectIndex = i;
	//					recentlyUsed.pushBack(std::move(cache));
	//				}
	//				return i;
	//			}
	//		}
	//		
	//		// Not found
	//		return SIZE_MAX;
	//	}
	//	
	//	// Create aspect from type indices with memory optimization
	//	template<typename... ComponentTypes>
	//	Aspect createAspect() {
	//		SboVector<std::type_index, 8> types;
	//		(types.push_back(std::type_index(typeid(ComponentTypes))), ...);
	//		
	//		Aspect aspect;
	//		aspect.types.reserve(types.size());
	//		for (const auto& type : types) {
	//			aspect.types.push_back(type);
	//		}
	//		
	//		// Sort and deduplicate
	//		eastl::sort(aspect.types.begin(), aspect.types.end());
	//		auto write_it = aspect.types.begin();
	//		for (auto read_it = aspect.types.begin(); read_it != aspect.types.end(); ++read_it) {
	//			if (write_it == aspect.types.begin() || *read_it != *(write_it - 1)) {
	//				*write_it = *read_it;
	//				++write_it;
	//			}
	//		}
	//		while (aspect.types.end() != write_it) {
	//			aspect.types.popBack();
	//		}
	//		
	//		return aspect;
	//	}
	//	
	//	// Get all registered aspects
	//	const global_vector<Aspect>& getAllAspects() const {
	//		return registeredAspects;
	//	}
	//	
	//	// Clear cache for memory cleanup
	//	void clearCache() {
	//		recentlyUsed.clear();
	//		commonAspects.clear();
	//	}
	//	
	//	// Get memory usage statistics
	//	struct MemoryStats {
	//		size_t registeredAspects;
	//		size_t commonAspects;
	//		size_t cacheEntries;
	//		size_t estimatedMemoryUsage;
	//	};
	//	
	//	MemoryStats getMemoryStats() const {
	//		MemoryStats stats;
	//		stats.registeredAspects = registeredAspects.size();
	//		stats.commonAspects = commonAspects.size();
	//		stats.cacheEntries = recentlyUsed.size();
	//		
	//		// Estimate memory usage (no allocator overhead thanks to global_vector/sbo_vector)
	//		stats.estimatedMemoryUsage = 
	//			sizeof(registeredAspects) + registeredAspects.capacity() * sizeof(Aspect) +
	//			sizeof(commonAspects) + commonAspects.capacity() * sizeof(Aspect) +
	//			sizeof(recentlyUsed) + recentlyUsed.capacity() * sizeof(AspectCache);
	//			
	//		return stats;
	//	}
//	};
}
