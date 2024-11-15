#include <Base/Logging.hpp>
#include <EASTL/list.h>
#include <EASTL/fixed_allocator.h>
#include <gtest/gtest.h>
#include <Base/Memory.hpp>


class BlockAllocatorTest : public testing::Test
{
protected:
	BlockAllocatorTest()
	{
		m_data.get_allocator().init(m_buffer, sizeof(m_buffer),
		                            sizeof(eastl::list<int, spite::BlockAllocator>::node_type),
		                            __alignof(eastl::list<int, spite::BlockAllocator>::node_type));
	}

	void TearDown() override
	{
		m_data.get_allocator().shutdown();
	}


	static constexpr sizet LIST_SIZE = 20;
	eastl::list<int, spite::BlockAllocator> m_data;
	eastl::list<int, spite::BlockAllocator>::node_type m_buffer[LIST_SIZE];
};


TEST_F(BlockAllocatorTest, AllocateDataAndCleanup)
{
	for (sizet i = 0; i < LIST_SIZE; ++i)
	{
		m_data.push_back(i);
	}
	sizet i = 0;
	for (auto data : m_data)
	{
		EXPECT_EQ(data, i);
		++i;
	}
	EXPECT_EQ(m_data.size(), LIST_SIZE);

	m_data.clear();
}

TEST_F(BlockAllocatorTest, ErorrOnUnclean)
{
	for (sizet i = 0; i < LIST_SIZE; ++i)
	{
		m_data.push_back(i);
	}

	EXPECT_ANY_THROW(m_data.get_allocator().shutdown());
	m_data.clear();
}
