#include <gtest/gtest.h>
#include "base/memory/HeapAllocator.hpp"
#include "base/memory/ScratchAllocator.hpp"
#include "base/memory/PoolAllocator.hpp"
#include "base/CollectionAliases.hpp"

// Test fixture for HeapAllocator
class HeapAllocatorTest : public testing::Test {
protected:
    void SetUp() override {
        //spite::initGlobalAllocator();
    }

    void TearDown() override {
        //spite::shutdownGlobalAllocator();
    }
};

TEST_F(HeapAllocatorTest, AllocationAndDeallocation) {
    spite::HeapAllocator allocator("TestHeap", 1 * spite::MB);
    void* block = allocator.allocate(128);
    ASSERT_NE(block, nullptr);
    allocator.deallocate(block);
    allocator.shutdown();
}

TEST_F(HeapAllocatorTest, AlignedAllocation) {
    spite::HeapAllocator allocator("TestHeap", 1 * spite::MB);
    void* block = allocator.allocate(128, 64);
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(block) % 64, 0);
    allocator.deallocate(block);
    allocator.shutdown();
}

TEST_F(HeapAllocatorTest, Reallocation) {
    spite::HeapAllocator allocator("TestHeap", 1 * spite::MB);
    void* block = allocator.allocate(128);
    ASSERT_NE(block, nullptr);
    block = allocator.reallocate(block, 256);
    ASSERT_NE(block, nullptr);
    allocator.deallocate(block);
    allocator.shutdown();
}

TEST_F(HeapAllocatorTest, NewDeleteObject) {
    spite::HeapAllocator allocator("TestHeap", 1 * spite::MB);
    auto* myObject = allocator.new_object<int>(42);
    ASSERT_NE(myObject, nullptr);
    ASSERT_EQ(*myObject, 42);
    allocator.delete_object(myObject);
    allocator.shutdown();
}

TEST_F(HeapAllocatorTest, LeakDetection) {
    spite::HeapAllocator allocator("TestHeap", 1 * spite::MB);
    void* block = allocator.allocate(128);
    ASSERT_NE(block, nullptr);
    // Expect a runtime error (assertion failure) due to leak
    ASSERT_THROW(allocator.shutdown(), std::runtime_error);
    allocator.shutdown(true); // Force shutdown
}

// Test fixture for ScratchAllocator
class ScratchAllocatorTest : public testing::Test {
protected:
    void SetUp() override {
        //spite::initGlobalAllocator();
    }

    void TearDown() override {
        //spite::shutdownGlobalAllocator();
    }
};

TEST_F(ScratchAllocatorTest, BasicAllocation) {
    spite::ScratchAllocator scratch(1 * spite::MB);
    void* p1 = scratch.allocate(100);
    void* p2 = scratch.allocate(200);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p1, p2);
}

TEST_F(ScratchAllocatorTest, Reset) {
    spite::ScratchAllocator scratch(1 * spite::MB);
    void* p1 = scratch.allocate(100);
    size_t used = scratch.bytes_used();
    scratch.reset();
    ASSERT_EQ(scratch.bytes_used(), 0);
    void* p2 = scratch.allocate(100);
    ASSERT_EQ(p1, p2); // Should allocate from the same spot
}

TEST_F(ScratchAllocatorTest, ScopedMarker) {
    spite::ScratchAllocator scratch(1 * spite::MB);
    scratch.allocate(100);
    size_t initial_used = scratch.bytes_used();
    {
        auto marker = scratch.get_scoped_marker();
        scratch.allocate(200);
        ASSERT_GT(scratch.bytes_used(), initial_used);
    }
    ASSERT_EQ(scratch.bytes_used(), initial_used);
}

TEST_F(ScratchAllocatorTest, OutOfMemory) {
    spite::ScratchAllocator scratch(1 * spite::KB);
    scratch.allocate(512);
    ASSERT_THROW(scratch.allocate(1024), std::runtime_error);
}

// Test fixture for PoolAllocator
class PoolAllocatorTest : public testing::Test {
protected:
    void SetUp() override {
        spite::initGlobalAllocator();
    }

    void TearDown() override {
        spite::PoolAllocator<int>::instance().cleanup();
        spite::shutdownGlobalAllocator();
    }
};

//TEST_F(PoolAllocatorTest, AllocateAndDeallocate) {
//    auto& pool = spite::PoolAllocator<int>::instance();
//    int* p1 = pool.allocate();
//    ASSERT_NE(p1, nullptr);
//    *p1 = 123;
//    pool.deallocate(p1);
//    int* p2 = pool.allocate();
//    ASSERT_EQ(p1, p2); // Should reuse the deallocated slot
//    pool.cleanup();
//}
//
//TEST_F(PoolAllocatorTest, MultipleAllocations) {
//    auto& pool = spite::PoolAllocator<int, 4>::instance();
//    int* p1 = pool.allocate();
//    int* p2 = pool.allocate();
//    int* p3 = pool.allocate();
//    int* p4 = pool.allocate();
//    ASSERT_NE(p1, nullptr);
//    ASSERT_NE(p2, nullptr);
//    ASSERT_NE(p3, nullptr);
//    ASSERT_NE(p4, nullptr);
//
//    // This should trigger a new block allocation
//    int* p5 = pool.allocate();
//    ASSERT_NE(p5, nullptr);
//}
