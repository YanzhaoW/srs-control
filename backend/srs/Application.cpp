#include "srs/Application.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/Connections.hpp"
#include "srs/utils/AppStatus.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/workflow/Handler.hpp"

#include <boost/asio/error.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <exception>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace srs
{
    internal::AppExitHelper::~AppExitHelper() noexcept { app_->end_of_work(); }

    App::App()
        : io_work_guard_{ asio::make_work_guard(io_context_) }
        , fec_strand_{ asio::make_strand(io_context_.get_executor()) }
    {
        spdlog::set_pattern("[%H:%M:%S] [%^%=7l%$] [thread %t] %v");
        spdlog::info("Welcome to SRS Application");
        workflow_handler_ = std::make_unique<workflow::Handler>(this);
    }

    void App::end_of_work()
    {
        spdlog::trace("Application: Resetting io context workguard ... ");
        io_work_guard_.reset();
        if (not status_.is_acq_off.load())
        {
            spdlog::critical(
                "Failed to close srs system! Please manually close the system with\n\nsrs_control --acq-off\n");
        }
        // TODO: set a timer
        //
        if (working_thread_.joinable())
        {
            spdlog::debug("Application: Wait until working thread finishes ...");
            // io_context_.stop();
            working_thread_.join();
            spdlog::debug("io context is stoped");
        }
        spdlog::debug("Application: working thread is finished");
        spdlog::debug("Application has exited.");
    }

    App::~App() noexcept
    {
        spdlog::debug("Calling the destructor of App ... ");
        signal_set_.cancel();

        // Turn off SRS data acquisition
        wait_for_reading_finish();

        if ((not is_switch_off_disaled_) and status_.is_acq_on.load())
        {
            spdlog::debug("Turning srs system off ...");
            switch_off();
        }
        else
        {
            set_status_acq_off(true);
        }
        set_status_acq_on(false);
    }

    void App::init()
    {
        set_remote_fec_endpoints();
        signal_set_.async_wait(
            [this](const boost::system::error_code& error, auto)
            {
                if (error == asio::error::operation_aborted)
                {
                    return;
                }
                exit();
                spdlog::info("Calling SIGINT from monitoring thread");
            });
        auto monitoring_action = [this]()
        {
            try
            {
                io_context_.join();
            }
            catch (const std::exception& ex)
            {
                spdlog::critical("Exception on working thread occured: {}", ex.what());
            }
        };
        working_thread_ = std::jthread{ monitoring_action };
    }

    void App::wait_for_reading_finish()
    {

        auto res = status_.wait_for_status(
            [](const auto& status)
            {
                spdlog::debug("Waiting for reading status false");
                return not status.is_reading.load();
            });

        if (not res)
        {
            spdlog::critical("TIMEOUT during waiting for status is_reading false.");
        }
    }

    // This will be called by Ctrl-C interrupt
    void App::exit()
    {
        spdlog::debug("App::exit is called");
        status_.is_on_exit.store(true);
        wait_for_reading_finish();
        workflow_handler_->stop();
    }

    void App::set_print_mode(common::DataPrintMode mode) { workflow_handler_->set_print_mode(mode); }
    void App::set_output_filenames(const std::vector<std::string>& filenames)
    {
        workflow_handler_->set_output_filenames(filenames);
    }

    void App::add_remote_fec_endpoint(std::string_view remote_ip, int port_number)
    {
        auto resolver = udp::resolver{ io_context_ };
        spdlog::debug("Add the remote FEC with ip: {} and port: {}", remote_ip, port_number);
        auto udp_endpoints = resolver.resolve(udp::v4(), remote_ip, fmt::format("{}", port_number));

        if (udp_endpoints.begin() == udp_endpoints.end())
        {
            spdlog::debug("Failed to add the FEC remote point");
            return;
        }
        remote_fec_endpoints_.push_back(*udp_endpoints.begin());
    }

    void App::switch_on()
    {
        auto connection_info = connection::Info{ this };
        connection_info.local_port_number = configurations_.fec_control_local_port;

        for (const auto& remote_endpoint : remote_fec_endpoints_)
        {
            auto fec_connection = std::make_shared<connection::Starter>(connection_info);
            fec_connection->set_remote_endpoint(remote_endpoint);
            asio::post(fec_strand_, [fec_connection = std::move(fec_connection)]() { fec_connection->acq_on(); });
        }
    }

    void App::switch_off()
    {
        const auto waiting_time = std::chrono::seconds{ 4 };
        auto is_ok = wait_for_status(
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
        auto connection_info = connection::Info{ this };
        connection_info.local_port_number = configurations_.fec_control_local_port;

        for (const auto& remote_endpoint : remote_fec_endpoints_)
        {
            auto fec_connection = std::make_shared<connection::Stopper>(connection_info);
            fec_connection->set_remote_endpoint(remote_endpoint);
            fec_connection->acq_off();
        }
        spdlog::info("SRS system is turned off");
        set_status_acq_off();
    }

    void App::read_data(bool is_non_stop)
    {
        auto connection_info = connection::Info{ this };
        connection_info.local_port_number = configurations_.fec_data_receive_port;
        data_reader_ = std::make_shared<connection::DataReader>(connection_info, workflow_handler_.get());
        data_reader_->start(is_non_stop);
    }

    void App::start_workflow(bool is_blocking) { workflow_handler_->start(is_blocking); }
    void App::wait_for_finish() { working_thread_.join(); }

    void App::set_remote_fec_endpoints()
    {
        for (const auto& fec_ips : configurations_.remote_fec_ips)
        {
            add_remote_fec_endpoint(fec_ips, common::DEFAULT_SRS_CONTROL_PORT);
        }
    }
} // namespace srs
