#include "srs/connections/Connections.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/workflow/Handler.hpp"

#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

namespace srs::connection
{
    void Starter::close()
    {
        close_socket();
        auto& control = get_app();
        control.set_status_acq_on();
        control.notify_status_change();
        if (is_connection_ok())
        {
            spdlog::info("Input data stream from the FEC with the IP: {} has been requested",
                         get_remote_endpoint().address().to_string());
        }
    }

    void Starter::send_message_from(std::shared_ptr<FecSwitchSocket> socket)
    {
        const auto data = std::vector<CommunicateEntryType>{ 0, 15, 1 };
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 1 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
        communicate(data, common::NULL_ADDRESS, std::move(socket));
        // socket.register_send_action();
    }

    void Starter::acq_on()
    {
        spdlog::info("Requesting data from the FEC with the IP: {}", get_remote_endpoint().address().to_string());
        const auto data = std::vector<CommunicateEntryType>{ 0, 15, 1 };
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 1 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
        communicate(data, common::NULL_ADDRESS);
    }

    void Stopper::send_message_from(std::shared_ptr<FecSwitchSocket> socket)
    {
        const auto data = std::vector<CommunicateEntryType>{ 0, 15, 0 };
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 0 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
        communicate(data, common::NULL_ADDRESS, std::move(socket));
        // communicate(data, common::NULL_ADDRESS);
    }

    void Stopper::acq_off()
    {
        const auto data = std::vector<CommunicateEntryType>{ 0, 15, 0 };
        const auto response_data = std::vector<CommunicateEntryType>{ 0, 0, 0 };
        set_write_response_msg(response_data, common::NULL_ADDRESS);
        communicate(data, common::NULL_ADDRESS);
    }

    void DataReader::close()
    {
        spdlog::debug("Stopping the UDP data reading ...");
        close_socket();
        auto& control = get_app();
        control.set_status_is_reading(false);
        spdlog::trace("Reading status is not false");
        control.notify_status_change();
        spdlog::info("UDP Data reading has been stopped");
    }

    void DataReader::read_data_handle(std::span<BufferElementType> read_data)
    {
        workflow_handler_->read_data_once(read_data);
    }
} // namespace srs::connection
