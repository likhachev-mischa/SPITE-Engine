
#include <gtest/gtest.h>
#include "base/Collections.hpp"
#include "base/memory/HeapAllocator.hpp"

class SboVectorTest : public testing::Test {
protected:
    spite::HeapAllocator allocator;

    SboVectorTest() : allocator("SboVectorTestAllocator", 1 * spite::MB) {}
    ~SboVectorTest() override {
        allocator.shutdown();
    }
};

TEST_F(SboVectorTest, InitialState) {
    spite::sbo_vector<int, 8> vec;
    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.capacity(), 8);
}

TEST_F(SboVectorTest, PushBackWithinInlineCapacity) {
    spite::sbo_vector<int, 8> vec;
    for (int i = 0; i < 8; ++i) {
        vec.push_back(i);
    }
    ASSERT_EQ(vec.size(), 8);
    ASSERT_EQ(vec.capacity(), 8);
    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(vec[i], i);
    }
}

TEST_F(SboVectorTest, PushBackExceedingInlineCapacity) {
    spite::sbo_vector<int, 4> vec;
    for (int i = 0; i < 5; ++i) {
        vec.push_back(i);
    }
    ASSERT_EQ(vec.size(), 5);
    ASSERT_GT(vec.capacity(), 4);
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(vec[i], i);
    }
}

TEST_F(SboVectorTest, EmplaceBack) {
    spite::sbo_vector<std::string, 2> vec;
    vec.emplace_back("hello");
    vec.emplace_back("world");
    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec[0], "hello");
    ASSERT_EQ(vec[1], "world");
}

TEST_F(SboVectorTest, PopBack) {
    spite::sbo_vector<int, 4> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.pop_back();
    ASSERT_EQ(vec.size(), 1);
    ASSERT_EQ(vec[0], 1);
}

TEST_F(SboVectorTest, Clear) {
    spite::sbo_vector<int, 4> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.clear();
    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);
}

TEST_F(SboVectorTest, Erase) {
    spite::sbo_vector<int, 8> vec;
    for (int i = 0; i < 8; ++i) {
        vec.push_back(i);
    }
    vec.erase(vec.begin() + 2);
    ASSERT_EQ(vec.size(), 7);
    ASSERT_EQ(vec[2], 3);
    vec.erase(vec.begin() + 3, vec.begin() + 5);
    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec[3], 6);
}

TEST_F(SboVectorTest, Resize) {
    spite::sbo_vector<int, 4> vec;
    vec.resize(2);
    ASSERT_EQ(vec.size(), 2);
    vec.resize(4, 5);
    ASSERT_EQ(vec.size(), 4);
    ASSERT_EQ(vec[3], 5);
}
