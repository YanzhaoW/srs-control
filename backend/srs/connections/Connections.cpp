#include "srs/connections/Connections.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>
#include <utility>
#include <vector>

namespace srs::connection
{
    Starter::Starter(std::string_view name)
        : Base{ name }
    {
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 1 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
    }
    Starter::Starter()
        : Starter{ "Starter" }
    {
    }

    void Starter::send_message_from(std::shared_ptr<FecCommandSocket> socket)
    {
        communicate(send_suffix_, common::NULL_ADDRESS, std::move(socket));
        // socket.register_send_action();
    }

    Stopper::Stopper(std::string_view name)
        : Base(name)
    {
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 0 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
    }

    Stopper::Stopper()
        : Stopper{ "Stopper" }
    {
    }

    void Stopper::send_message_from(std::shared_ptr<FecCommandSocket> socket)
    {
        communicate(send_suffix_, common::NULL_ADDRESS, std::move(socket));
    }
} // namespace srs::connection
