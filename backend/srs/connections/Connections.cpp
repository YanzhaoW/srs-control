#include "srs/connections/Connections.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

namespace srs::connection
{
    Starter::Starter(const Config& config)
        : Base{ config }
    {
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 1 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
    }
    Starter::Starter()
        : Starter{ { .name = "Starter" } }
    {
    }

    void Starter::send_message_from(std::shared_ptr<FecSwitchSocket> socket)
    {
        communicate(send_suffix_, common::NULL_ADDRESS, std::move(socket));
        // socket.register_send_action();
    }

    Stopper::Stopper(const Config& config)
        : Base(config)
    {
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 0 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
    }

    Stopper::Stopper()
        : Stopper{ { .name = "Stopper" } }
    {
    }

    void Stopper::send_message_from(std::shared_ptr<FecSwitchSocket> socket)
    {
        communicate(send_suffix_, common::NULL_ADDRESS, std::move(socket));
    }
} // namespace srs::connection
