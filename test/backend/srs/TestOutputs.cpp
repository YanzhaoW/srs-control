#include <filesystem>
#include <thread>

#include <gtest/gtest.h>

#include <srs/SRSEmulator.hpp>
#include <srs/utils/Connections.hpp>
#include <srs/workflow/Handler.hpp>

namespace
{
    class Runner
    {
      public:
        Runner() = default;

        [[nodiscard]] auto get_error_msg() const -> const auto& { return error_msg_; }
        [[nodiscard]] auto get_event_nums() const -> const auto& { return event_nums_; }

        void run(const std::vector<std::string>& output_filenames)
        {

            try
            {
                auto app = srs::App{};

                app.set_fec_data_receiv_port(0);
                app.set_output_filenames(output_filenames);

                app.init();
                app.read_data(false);
                auto* data_reader = app.get_data_reader();
                auto port_num = data_reader->get_local_port_number();
                auto emulator = srs::test::SRSEmulator{ "test_data.bin", port_num, app };

                auto analysis_thread = std::jthread(
                    [&app, this]()
                    {
                        app.start_workflow(false);
                        event_nums_ = app.get_workflow_handler().get_processed_hit_number();
                    });

                // std::this_thread::sleep_for(std::chrono::seconds(1));
                emulator.start_send_data();
                error_msg_ = app.get_error_string();
            }
            catch (std::exception& err)
            {
                spdlog::critical("Exception occurred: {}", err.what());
            }
        }

      private:
        std::string error_msg_;
        uint64_t event_nums_ = 0;
    };

    namespace fs = std::filesystem;

    auto check_if_file_exist(const std::string& filename) -> bool
    {
        auto file_path = fs::path{ filename };
        return fs::exists(file_path);
    }

} // namespace

TEST(integration_test_outputfiles, binary_output)
{
    const auto filename = std::string{ "test_output.bin" };

    auto runner = Runner{};
    ASSERT_NO_THROW(runner.run(std::vector{ filename }));
    EXPECT_EQ(runner.get_error_msg(), "");
    EXPECT_EQ(runner.get_event_nums(), 0);

    auto res = check_if_file_exist(filename);
    ASSERT_TRUE(res);
}

TEST(integration_test_outputfiles, root_output)
{
    const auto filename = std::string{ "test_output.root" };

    auto runner = Runner{};
    ASSERT_NO_THROW(runner.run(std::vector{ filename }));
    EXPECT_EQ(runner.get_error_msg(), "");
    EXPECT_GT(runner.get_event_nums(), 0);

    auto res = check_if_file_exist(filename);
#ifdef HAS_ROOT
    ASSERT_TRUE(res);
#else
    ASSERT_FALSE(res);
#endif
}

TEST(integration_test_outputfiles, proto_binary_output)
{
    const auto filename = std::string{ "test_output.binpb" };

    auto runner = Runner{};
    ASSERT_NO_THROW(runner.run(std::vector{ filename }));
    EXPECT_EQ(runner.get_error_msg(), "");
    EXPECT_GT(runner.get_event_nums(), 0);

    auto res = check_if_file_exist(filename);
    ASSERT_TRUE(res);
}

TEST(integration_test_outputfiles, json_output)
{
    const auto filename = std::string{ "test_output.json" };

    auto runner = Runner{};
    ASSERT_NO_THROW(runner.run(std::vector{ filename }));
    EXPECT_EQ(runner.get_error_msg(), "");
    EXPECT_GT(runner.get_event_nums(), 0);

    auto res = check_if_file_exist(filename);
#ifdef HAS_ROOT
    ASSERT_TRUE(res);
#else
    ASSERT_FALSE(res);
#endif
}

TEST(integration_test_outputfiles, all_outputs)
{
    const auto filenames = std::vector<std::string>{
        "test_output1.bin", "test_output2.bin", "test_output1.json", "test_output1.root", "test_output1.binpb"
    };

    auto runner = Runner{};
    ASSERT_NO_THROW(runner.run(filenames));
    EXPECT_EQ(runner.get_error_msg(), "");
    EXPECT_GT(runner.get_event_nums(), 0);

    for (const auto& filename : filenames)
    {
        auto res = check_if_file_exist(filename);
        if (filename == "test_output1.root")
        {
#ifdef HAS_ROOT
            ASSERT_TRUE(res);
#else
            ASSERT_FALSE(res);
#endif
        }
        else
        {
            ASSERT_TRUE(res);
        }
    }
}
