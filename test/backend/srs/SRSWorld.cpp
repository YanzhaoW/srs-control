#include "SRSWorld.hpp"
#include "srs/Application.hpp"
#include "srs/SRSEmulator.hpp"
#include "srs/utils/ExitLogger.hpp"
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace srs::test
{
    World::World(App& app, std::string_view input_filename)
        : app_{ &app }
    {
        const auto& app_config = app.get_config();
        for (const auto port_num : app_config.fec_data_receive_ports)
        {
            srs_server_.push_back(

                std::make_unique<SRSEmulator>(SRSEmulator::Config{ .data_port = port_num,
                                                                   .listen_port = app_config.fec_control_remote_port,
                                                                   .write_port = app_config.fec_control_local_port,
                                                                   .filename = input_filename }));
        }
    }

    void World::init() { app_->init(); }

    auto World::launch_server() -> std::optional<std::string>
    {
        for (auto& server : srs_server_)
        {

            srs_server_thread_.emplace_back(
                [this, &server]()
                {
                    server->wait_for_connection();
                    spdlog::trace("World: Server stops waiting for connections.");
                });
        }
        return {};
    }

    void World::join()
    {
        auto _ = ExitLogger{};
        for (auto& thread : srs_server_thread_)
        {
            thread.join();
        }
    }
} // namespace srs::test
