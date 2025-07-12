#include <gtest/gtest.h>
#include "GlobalTestEnvironment.hpp"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new spite::GlobalEnvironment);
    return RUN_ALL_TESTS();
}
