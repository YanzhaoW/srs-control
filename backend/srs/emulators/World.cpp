#include "World.hpp"
#include "srs/emulators/FECEmulator.hpp"
#include <cstdint>
#include <memory>
#include <ranges>

namespace srs::emulator
{
    void World::init()
    {
        const auto& fec_configs = config_.FECs;

        for (const auto& [fec_id, fec_config] : std::views::zip(std::views::iota(uint8_t{}), fec_configs))
        {
            auto& server = fec_servers_.emplace_back(std::make_unique<FECEmulator>(fec_config, *this, io_context_));
            server->set_delay_time(config_.frame_wait_time_ns);
            server->set_fec_id(fec_id);
        }
    }

    void World::launch()
    {
        for (auto& server : fec_servers_)
        {
            server->wait_for_connection();
        }
        io_context_.join();
    }
} // namespace srs::emulator
