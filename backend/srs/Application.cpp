#include "srs/Application.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/Connections.hpp"
#include "srs/connections/DataSocket.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/connections/SpecialSocketBase.hpp"
#include "srs/utils/AppStatus.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/workflow/Handler.hpp"

#include <boost/asio/error.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/detail/errc.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/thread/futures/future_error_code.hpp>
#include <chrono>
#include <cstddef>
#include <exception>
#include <expected>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <future>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

namespace srs
{
    internal::AppExitHelper::~AppExitHelper() noexcept { app_->action_after_destructor(); }

    App::App()
        : io_work_guard_{ asio::make_work_guard(io_context_) }
        , fec_strand_{ asio::make_strand(io_context_.get_executor()) }
    {
        spdlog::set_pattern("[%H:%M:%S][%^%=6l%$][%t] %v");
        spdlog::info("Welcome to SRS Application");
        workflow_handler_ = std::make_unique<workflow::Handler>(this);
    }

    void App::action_after_destructor()
    {
        spdlog::trace("Application: Resetting io context workguard ... ");
        io_work_guard_.reset();
        // if (not status_.is_acq_off.load())
        // {
        //     spdlog::critical(
        //         "Failed to close srs system! Please manually close the system with\n\n srs_control --acq-off\n");
        // }
        if (auto switch_off_status = wait_for_switch_action(switch_off_future_);
            switch_off_status == std::future_status::ready)
        {
            spdlog::info("Application: FECs has been switched off successfully.");
        }
        else if (switch_off_status == std::future_status::timeout)
        {
            spdlog::warn("Application: TIMEOUT while waiting for switching off process to finish.");
        }
        else if (switch_off_status == std::future_status::deferred)
        {
            spdlog::warn("Application: Switching off of FECs is pending.");
        }
        else if (not switch_off_status.has_value())
        {
            spdlog::debug("Application: FECs don't need switching off due to: {}", switch_off_status.error().message());
        }

        // TODO: set a timer
        //
        if (working_thread_.joinable())
        {
            spdlog::debug("Application: Wait until working thread finishes ...");
            // io_context_.stop();
            working_thread_.join();
            spdlog::debug("Application: All tasks in main io_context are finished.");
        }
        spdlog::debug("Application has exited.");
    }

    App::~App() noexcept
    {
        spdlog::debug("Application: Calling the destructor ");
        signal_set_.cancel();
        signal_set_.clear();
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
                exit_and_switch_off();
                spdlog::info("Application: Calling SIGINT from monitoring thread");
            });
        auto monitoring_action = [this]()
        {
            try
            {
                io_context_.join();
            }
            catch (const std::exception& ex)
            {
                spdlog::critical("Application: Exception on working thread occurred: {}", ex.what());
            }
        };
        working_thread_ = std::jthread{ monitoring_action };
    }

    void App::wait_for_reading_finish()
    {
        if (data_socket_ == nullptr)
        {
            return;
        }
        data_socket_->cancel();
        auto status = data_socket_->wait_for_listen_finish(common::DEFAULT_STATUS_WAITING_TIME_SECONDS);
        if (status.has_value() and status.value() == std::future_status::timeout)
        {
            spdlog::warn("Application: TIMEOUT during waiting for the closing of input data stream.");
        }
    }

    // This will be called by Ctrl-C interrupt
    void App::exit_and_switch_off()
    {
        fmt::println("");
        spdlog::debug("Application: exit is called");
        status_.is_on_exit.store(true);
        wait_for_reading_finish();
        workflow_handler_->stop();

        // Turn off SRS data acquisition
        wait_for_reading_finish();

        if (auto switch_on_status = wait_for_switch_action(switch_on_future_);
            switch_on_status == std::future_status::ready)
        {
            spdlog::debug("Application: FECs has been switched on successfully.");
            switch_off();
        }
        else if (switch_on_status == std::future_status::timeout)
        {
            spdlog::warn(
                "Application: TIMEOUT during waiting for FECs to be switched on. Skipping switching off process.");
        }
        else if (switch_on_status == std::future_status::deferred)
        {
            spdlog::warn("Application: Switching on of FECs is pending. Skipping switching off process.");
        }
        else
        {
            spdlog::info("Application: FECs were not switched on. Skipping switching off process.");
        }
    }

    void App::set_print_mode(common::DataPrintMode mode) { workflow_handler_->set_print_mode(mode); }
    void App::set_output_filenames(const std::vector<std::string>& filenames, std::size_t n_lines)
    {
        workflow_handler_->set_n_pipelines(n_lines);
        workflow_handler_->set_output_filenames(filenames);
    }

    void App::add_remote_fec_endpoint(std::string_view remote_ip, int port_number)
    {
        auto resolver = udp::resolver{ io_context_ };
        spdlog::info("Application: Add the remote FEC with ip: {} and port: {}", remote_ip, port_number);
        auto udp_endpoints = resolver.resolve(udp::v4(), remote_ip, fmt::format("{}", port_number));

        if (udp_endpoints.begin() == udp_endpoints.end())
        {
            spdlog::debug("Application: Failed to add the FEC remote point");
            return;
        }
        remote_fec_endpoints_.push_back(*udp_endpoints.begin());
    }

    template <typename T>
    auto App::switch_FECs(std::string_view connection_name) -> SwitchFutureType
    {
        return connection::SpecialSocket::create<connection::FecSwitchSocket>(config_.fec_control_local_port,
                                                                              io_context_)
            .transform(
                [this, connection_name](const std::shared_ptr<connection::FecSwitchSocket>& socket)
                {
                    for (const auto& remote_endpoint : remote_fec_endpoints_)
                    {
                        auto fec_connection = std::make_shared<T>(
                            connection::Base::Config{ .name = std::string{ connection_name },
                                                      .buffer_size = common::SMALL_READ_MSG_BUFFER_SIZE });
                        fec_connection->set_remote_endpoint(remote_endpoint);
                        fec_connection->send_message_from(socket);
                    }
                    auto res_fut = socket->cancel_listen_after(io_context_);
                    socket->launch_actions();
                    return res_fut;
                });
    }

    void App::switch_on()
    {
        spdlog::info("Application: Switching on FEC devices ...");
        switch_on_future_ = switch_FECs<connection::Starter>("Starter");
    }

    void App::switch_off()
    {
        spdlog::info("Application: Switching off FEC devices ...");
        switch_off_future_ = switch_FECs<connection::Stopper>("Stopper");
    }

    void App::read_data(bool /*is_non_stop*/)
    {
        spdlog::info("Application: Starting input data stream ...");
        auto fut = connection::SpecialSocket::create<connection::DataSocket>(
                       config_.fec_data_receive_port, io_context_, workflow_handler_.get())
                       .transform([this](auto socket) { data_socket_ = std::move(socket); });
        spdlog::debug("data stream is using the buffer size: {}", config_.data_buffer_size);
        data_socket_->set_buffer_size(config_.data_buffer_size);

        if (not fut.has_value())
        {
            spdlog::critical(
                "Application: Cannot establish the connection for the input data stream because the local port number "
                "{} is not available.",
                config_.fec_data_receive_port);
        }
    }

    void App::start_workflow()
    {
        workflow_thread_ = std::jthread(
            [this]()
            {
                spdlog::info("Application: Starting input data analysis workflow ...");
                workflow_handler_->start();
            });
    }

    void App::wait_for_finish() { working_thread_.join(); }

    auto App::wait_for_switch_action(const SwitchFutureType& switch_future) -> SwitchFutureStatusType
    {
        if (not switch_future->valid())
        {
            return std::unexpected{ asio::error::make_error_code(asio::error::basic_errors::invalid_argument) };
        }
        return switch_future.transform(
            [](const std::future<void>& switch_fut)
            {
                if (switch_fut.valid())
                {
                    return switch_fut.wait_for(common::DEFAULT_STATUS_WAITING_TIME_SECONDS);
                };
                return std::future_status::ready;
            });
    }

    void App::set_remote_fec_endpoints()
    {
        for (const auto& fec_ips : config_.remote_fec_ips)
        {
            add_remote_fec_endpoint(fec_ips, common::DEFAULT_SRS_CONTROL_PORT);
        }
    }
} // namespace srs
