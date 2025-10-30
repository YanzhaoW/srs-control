#include <chrono>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>

#include "SRSWorld.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace
{
    namespace fs = std::filesystem;
    auto check_if_file_exist(const std::string& filename) -> bool
    {
        auto file_path = fs::path{ filename };
        return fs::exists(file_path);
    }
    using srs::test::World;

} // namespace

TEST(integration_test_outputfiles, no_output)
{
    auto is_continuous_output = false;
    spdlog::set_level(spdlog::level::trace);
    auto world = World{ "test_data.bin" };
    world.set_continue_output(is_continuous_output);
    world.launch_server();

    auto& app = world.get_app();

    auto output_file = std::vector<std::string>{ "test_output.bin" };
    app.set_output_filenames(output_file);

    app.read_data();
    app.switch_on();

    auto switch_off_thread = std::jthread(
        [&app, &world, is_continuous_output]()
        {
            if (is_continuous_output)
            {
                std::this_thread::sleep_for(std::chrono::seconds{ 5 });
            }
            else
            {
                world.get_server()->wait_for_data_sender();
            }
            app.exit_and_switch_off();
        });

    app.start_workflow();
}

// TEST(integration_test_outputfiles, binary_output)
// {
//     const auto filename = std::string{ "test_output.bin" };

//     auto runner = World{};
//     ASSERT_NO_THROW(runner.run(std::vector{ filename }));
//     EXPECT_EQ(runner.get_error_msg(), "");
//     EXPECT_EQ(runner.get_event_nums(), 0);

//     auto res = check_if_file_exist(filename);
//     ASSERT_TRUE(res);
// }

// TEST(integration_test_outputfiles, root_output)
// {
//     const auto filename = std::string{ "test_output.root" };

//     auto runner = World{};
//     ASSERT_NO_THROW(runner.run(std::vector{ filename }));
//     EXPECT_EQ(runner.get_error_msg(), "");
//     EXPECT_GT(runner.get_event_nums(), 0);

//     auto res = check_if_file_exist(filename);
// #ifdef HAS_ROOT
//     ASSERT_TRUE(res);
// #else
//     ASSERT_FALSE(res);
// #endif
// }

// TEST(integration_test_outputfiles, proto_binary_output)
// {
//     const auto filename = std::string{ "test_output.binpb" };

//     auto runner = World{};
//     ASSERT_NO_THROW(runner.run(std::vector{ filename }));
//     EXPECT_EQ(runner.get_error_msg(), "");
//     EXPECT_GT(runner.get_event_nums(), 0);

//     auto res = check_if_file_exist(filename);
//     ASSERT_TRUE(res);
// }

// TEST(integration_test_outputfiles, json_output)
// {
//     const auto filename = std::string{ "test_output.json" };

//     auto runner = World{};
//     ASSERT_NO_THROW(runner.run(std::vector{ filename }));
//     EXPECT_EQ(runner.get_error_msg(), "");
//     EXPECT_GT(runner.get_event_nums(), 0);

//     auto res = check_if_file_exist(filename);
// #ifdef HAS_ROOT
//     ASSERT_TRUE(res);
// #else
//     ASSERT_FALSE(res);
// #endif
// }

// TEST(integration_test_outputfiles, all_outputs)
// {
//     const auto filenames = std::vector<std::string>{
//         "test_output1.bin", "test_output2.bin", "test_output1.json", "test_output1.root", "test_output1.binpb"
//     };

//     auto runner = World{};
//     ASSERT_NO_THROW(runner.run(filenames));
//     EXPECT_EQ(runner.get_error_msg(), "");
//     EXPECT_GT(runner.get_event_nums(), 0);

//     for (const auto& filename : filenames)
//     {
//         auto res = check_if_file_exist(filename);
//         if (filename == "test_output1.root")
//         {
// #ifdef HAS_ROOT
//             ASSERT_TRUE(res);
// #else
//             ASSERT_FALSE(res);
// #endif
//         }
//         else
//         {
//             ASSERT_TRUE(res);
//         }
//     }
// }
