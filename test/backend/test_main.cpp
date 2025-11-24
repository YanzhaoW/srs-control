#include <gtest/gtest.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
