#include <srs/utils/Connections.hpp>
#include <srs/workflow/Handler.hpp>

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

    void Stopper::acq_off()
    {
        const auto data = std::vector<CommunicateEntryType>{ 0, 15, 0 };
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
