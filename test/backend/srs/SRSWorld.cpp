#include "SRSWorld.hpp"
#include "srs/Application.hpp"
#include "srs/SRSEmulator.hpp"
#include "srs/utils/ExitLogger.hpp"
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace srs::test
{
    World::World(App& app, std::string_view input_filename)
        : app_{ &app }
        , input_filename_{ input_filename }
    {
    }

    void World::init()
    {
        auto& app_config = app_->get_config_ref();
        const auto ips = std::views::zip(std::views::iota(1), app_config.fec_data_receive_ports) | std::views::keys |
                         std::views::transform([](auto idx) { return std::format("127.0.0.{}", idx); }) |
                         std::ranges::to<std::vector<std::string>>();
        app_config.remote_fec_ips = ips;
        for (const auto [ip, port_num] : std::views::zip(ips, app_config.fec_data_receive_ports))
        {
            spdlog::info("World: Adding a srs server with the port number: {}", port_num);
            auto& server = srs_servers_.emplace_back(
                std::make_unique<SRSEmulator>(SRSEmulator::Config{ .data_port = port_num,
                                                                   .listen_port = app_config.fec_control_remote_port,
                                                                   .write_port = app_config.fec_control_local_port,
                                                                   .filename = input_filename_,
                                                                   .ip = ip },
                                              io_context_));
            server->set_continue_output(config_.is_continue);
            server->set_delay_time(config_.delay_time);
        }

        app_->init();
    }

    auto World::launch_server() -> std::optional<std::string>
    {
        for (auto& server : srs_servers_)
        {
            const auto tag = std::format("{}", server->get_config().ip);
            auto _ = ExitLogger{ tag };
            server->wait_for_connection();
            spdlog::trace("World: Server stops waiting for connections.");
            // srs_server_thread_.emplace_back([this, &server, current_loc]() {});
        }
        return {};
    }

    void World::join()
    {
        auto _ = ExitLogger{};
        io_context_.join();
        // for (auto& thread : srs_server_thread_)
        // {
        //     thread.join();
        // }
    }
} // namespace srs::test
