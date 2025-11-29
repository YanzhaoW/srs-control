#include "SRSWorld.hpp"
#include "srs/SRSEmulator.hpp"
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <thread>

namespace srs::test
{
    World::World(std::string_view input_filename)
        : app_config_{ app_.get_config_ref() }
    {
        srs_server_ =
            std::make_unique<SRSEmulator>(SRSEmulator::Config{ .data_port = app_config_.fec_data_receive_port,
                                                               .listen_port = app_config_.fec_control_remote_port,
                                                               .write_port = app_config_.fec_control_local_port,
                                                               .filename = input_filename });
    }

    void World::init() { app_.init(); }

    auto World::launch_server() -> std::optional<std::string>
    {
        srs_server_thread_ = std::jthread(
            [this]()
            {
                srs_server_->wait_for_connection();
                spdlog::trace("World: Server stops waiting for connections.");
            });
        return {};
    }

} // namespace srs::test
