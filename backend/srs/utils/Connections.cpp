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
        spdlog::info("SRS system is turned on");
    }

    void Stopper::acq_off()
    {
        const auto waiting_time = std::chrono::seconds{ 4 };
        auto is_ok = get_app().wait_for_status(
            [](const Status& status)
            {
                spdlog::debug("Waiting for acq_on status true ...");
                return status.is_acq_on.load();
            },
            waiting_time);

        if (not is_ok)
        {
            throw std::runtime_error("TIMEOUT during waiting for status is_acq_on true.");
        }
        const auto data = std::vector<CommunicateEntryType>{ 0, 15, 0 };
        communicate(data, common::NULL_ADDRESS);
    }

    void DataReader::close()
    {
        spdlog::debug("Stopping UDP data reading ...");
        close_socket();
        auto& control = get_app();
        control.set_status_is_reading(false);
        spdlog::trace("reading status is not false");
        control.notify_status_change();
        spdlog::info("UDP Data reading is stopped.");
    }

    void DataReader::read_data_handle(std::span<BufferElementType> read_data)
    {

        workflow_handler_->read_data_once(read_data);
    }
} // namespace srs::connection
