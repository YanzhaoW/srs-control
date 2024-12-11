#include <string_view>

#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <srs/Application.hpp>
#include <srs/utils/Connections.hpp>
#include <srs/workflow/Handler.hpp>

namespace srs
{
    App::App()
        : io_work_guard_{ asio::make_work_guard(io_context_) }
    {
        spdlog::set_pattern("[%H:%M:%S] [%^%=7l%$] [thread %t] %v");
        spdlog::info("Welcome to SRS Application");
        workflow_handler_ = std::make_unique<workflow::Handler>(this);
    }

    AppExitHelper::~AppExitHelper() noexcept { app_->end_of_work(); }

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

        if (status_.is_acq_on.load())
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
        signal_set_.async_wait(
            [this](const boost::system::error_code& error, auto)
            {
                if (error == asio::error::operation_aborted)
                {
                    return;
                }
                exit();
                spdlog::info("calling SIGINT from monitoring thread");
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

    void App::set_remote_endpoint(std::string_view remote_ip, int port_number)
    {
        auto resolver = udp::resolver{ io_context_ };
        spdlog::debug("Set the remote socket with ip: {} and port: {}", remote_ip, port_number);
        auto udp_endpoints = resolver.resolve(udp::v4(), remote_ip, fmt::format("{}", port_number));
        remote_endpoint_ = *udp_endpoints.begin();
    }

    void App::switch_on()
    {
        auto connection_info = connection::Info{ this };
        connection_info.local_port_number = configurations_.fec_control_local_port;
        auto connection = std::make_shared<connection::Starter>(connection_info);
        connection->set_remote_endpoint(remote_endpoint_);
        connection->acq_on();
    }

    void App::switch_off()
    {
        auto connection_info = connection::Info{ this };
        connection_info.local_port_number = configurations_.fec_control_local_port;
        auto connection = std::make_shared<connection::Stopper>(connection_info);
        connection->set_remote_endpoint(remote_endpoint_);
        connection->acq_off();
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
} // namespace srs
