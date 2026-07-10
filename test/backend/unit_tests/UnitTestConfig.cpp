#include "srs/ConfigHandler.hpp"
#include "srs/devices/Configuration.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <vector>

TEST_CASE("ConfigHandler")
{
    auto app_config = srs::Config{};

    SECTION("write")
    {
        auto filename = std::string{};
        SECTION("yaml") { filename = "test_config_output.yaml"; }
        SECTION("json") { filename = "test_config_output.json"; }
        auto is_not_ok = srs::config::output_config_to_file(app_config, filename, true);

        if (is_not_ok)
        {
            UNSCOPED_INFO(is_not_ok.value());
        }
        REQUIRE_FALSE(is_not_ok);
        auto res = srs::common::get_file_size(filename);
        if (not res)
        {
            UNSCOPED_INFO(fmt::format("get file size error: {}", res.error()));
        }
        REQUIRE(res);
        CHECK(res.value() > 0);
    }

    SECTION("read")
    {
        auto filename = std::string{};
        SECTION("yaml") { filename = TEST_YAML_CONFIG_INPUT_FILENAME; }
        SECTION("json") { filename = TEST_JSON_CONFIG_INPUT_FILENAME; }

        auto is_not_ok = srs::config::set_config_from_file(app_config, filename, false);
        if (is_not_ok)
        {
            UNSCOPED_INFO(is_not_ok.value());
        }
        REQUIRE_FALSE(is_not_ok);

        CHECK(app_config.fec_control_local_port == 6017);
        CHECK(app_config.fec_control_remote_port == 6610);
        CHECK(app_config.fec_data_receive_ports == std::vector{ 6001 });
        CHECK(app_config.data_buffer_size == 6000);
        CHECK(app_config.remote_fec_ips == std::vector{ std::string{ "10.0.0.100" } });
        CHECK(app_config.buffer_queue_capacity == 100);
        CHECK(app_config.output_filenames == std::vector{ std::string{ "test.output" } });
        CHECK(app_config.output_split == 2);
        CHECK(app_config.time_wait_after_acq_off_ms == 100);
        CHECK(app_config.warn_if_data_drop);
        CHECK(magic_enum::enum_name(app_config.data_print_mode) == std::string{ "print_raw" });
    }
}
