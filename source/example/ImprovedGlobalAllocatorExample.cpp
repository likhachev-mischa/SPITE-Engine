// Example demonstrating the improved global allocator usage patterns
// This shows how the new implementation solves common problems

#include "base/memory/HeapAllocator.hpp"
#include "base/Logging.hpp"

namespace spite {

// EXAMPLE 1: Safe static initialization order
// This global object can safely use the allocator during construction
class ExampleGlobalSystem {
private:
    std::vector<int, GlobalHeapAllocator<int>> data_;
    
public:
    ExampleGlobalSystem() {
        // Safe! Global allocator is guaranteed to be initialized
        // before this constructor runs due to init_order(101)
        data_.reserve(1000);
        SDEBUG_LOG("ExampleGlobalSystem initialized with global allocator\n");
    }
};

// This will work correctly - no initialization order issues
static ExampleGlobalSystem g_exampleSystem;

// EXAMPLE 2: Application shutdown with memory leak detection
void shutdownApplication() {
    SDEBUG_LOG("Shutting down application...\n");
    
    // Optional: Check for memory leaks in debug builds
#ifdef DEBUG
    shutdownGlobalAllocator(false); // Will report leaks
#else
    shutdownGlobalAllocator(true);  // Clean shutdown without checks
#endif
}

// EXAMPLE 3: Performance comparison demonstration
void performanceComparison() {
    const size_t iterations = 1000000;
    
    // OLD WAY (Meyer's singleton) - had potential blocking on first access
    // + thread synchronization overhead on every call
    
    // NEW WAY - direct global variable access
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        auto& allocator = getGlobalAllocator(); // Very fast - just returns reference
        (void)allocator; // Suppress unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    SDEBUG_LOG("Global allocator access time for %zu iterations: %lld microseconds\n", 
               iterations, duration.count());
}

// EXAMPLE 4: Using with different container types
void containerExamples() {
    // Vector with global allocator
    std::vector<int, GlobalHeapAllocator<int>> global_vector;
    global_vector.push_back(42);
    
    // String with global allocator  
    std::basic_string<char, std::char_traits<char>, GlobalHeapAllocator<char>> global_string;
    global_string = "Hello, global allocator!";
    
    // Custom container
    eastl::vector<float, GlobalHeapAllocator<float>> eastl_vector;
    eastl_vector.push_back(3.14f);
    
    SDEBUG_LOG("All containers using global allocator successfully\n");
}

}

/*
KEY IMPROVEMENTS SUMMARY:

1. INITIALIZATION ORDER CONTROL:
   ✅ Uses init_order(101) for predictable startup
   ✅ Compatible with Tracy's initialization pattern  
   ✅ Works across GCC, Clang, and MSVC

2. PERFORMANCE IMPROVEMENTS:
   ✅ No more atomic operations on access
   ✅ No blocking on first use
   ✅ Simple global variable access

3. THREAD SAFETY:
   ✅ Initialized before main() starts
   ✅ No race conditions during startup
   ✅ Safe for use in global constructors

4. MAINTAINABILITY:
   ✅ Single source of truth (HeapAllocator.cpp)
   ✅ No duplication between headers
   ✅ Clean forward declarations

5. DEBUG SUPPORT:
   ✅ Configurable size based on build type
   ✅ Optional shutdown with leak detection
   ✅ Named allocator for debugging

MIGRATION PATH:
- Existing code using getGlobalAllocator() works unchanged
- GlobalHeapAllocator<T> containers work unchanged  
- Only internal implementation improved
- No breaking changes to public API
*/
