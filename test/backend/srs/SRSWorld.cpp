#include "SRSWorld.hpp"
#include "srs/Application.hpp"
#include "srs/SRSEmulator.hpp"
#include "srs/utils/ExitLogger.hpp"
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace srs::test
{
    World::World(App& app, const Config& config)
        : app_{ &app }
        , config_{ config }
    {
    }

    void World::init()
    {
        auto& app_config = app_->get_config_ref();
        if (config_.n_instances > 1)
        {
            const auto ips = std::views::iota(std::size_t{ 1 }, 1 + config_.n_instances) |
                             std::views::transform([](auto idx) { return std::format("127.0.0.{}", idx); }) |
                             std::ranges::to<std::vector<std::string>>();
            app_config.remote_fec_ips = ips;
            const auto port_num = app_config.fec_data_receive_ports.front();
            for (const auto [idx, ip] : std::views::zip(std::views::iota(0), ips))
            {
                spdlog::info("World: Adding a srs server with the port number: {} and ip: {}", port_num, ip);
                auto& server = srs_servers_.emplace_back(std::make_unique<SRSEmulator>(
                    SRSEmulator::Config{ .data_port = port_num,
                                         .listen_port = app_config.fec_control_remote_port,
                                         .write_port = app_config.fec_control_local_port,
                                         .server_idx = idx,
                                         .n_servers = config_.n_instances,
                                         .filename = config_.input_filename,
                                         .ip = ip },
                    io_context_));
                server->set_continue_output(config_.is_continue);
                server->set_delay_time(config_.delay_time);
            }
        }
        else
        {
            const auto ips = std::views::zip(std::views::iota(1), app_config.fec_data_receive_ports) |
                             std::views::keys |
                             std::views::transform([](auto idx) { return std::format("127.0.0.{}", idx); }) |
                             std::ranges::to<std::vector<std::string>>();
            app_config.remote_fec_ips = ips;
            for (const auto [ip, port_num] : std::views::zip(ips, app_config.fec_data_receive_ports))
            {
                spdlog::info("World: Adding a srs server with the port number: {}", port_num);
                auto& server = srs_servers_.emplace_back(std::make_unique<SRSEmulator>(
                    SRSEmulator::Config{ .data_port = port_num,
                                         .listen_port = app_config.fec_control_remote_port,
                                         .write_port = app_config.fec_control_local_port,
                                         .filename = config_.input_filename,
                                         .ip = ip },
                    io_context_));
                server->set_continue_output(config_.is_continue);
                server->set_delay_time(config_.delay_time);
            }
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
