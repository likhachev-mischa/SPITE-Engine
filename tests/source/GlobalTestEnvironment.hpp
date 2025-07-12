#pragma once
#include <gtest/gtest.h>

namespace spite
{
    class GlobalEnvironment : public ::testing::Environment
    {
    public:
        ~GlobalEnvironment() override {}

        void SetUp() override;
        void TearDown() override;
    };
}
