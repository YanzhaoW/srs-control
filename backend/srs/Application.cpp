#include "srs/Application.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/Connections.hpp"
#include "srs/connections/DataSocket.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/connections/SpecialSocketBase.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/workflow/Handler.hpp"

#include "spdlog/sinks/rotating_file_sink.h"
#include <algorithm>
#include <boost/asio/error.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <cstddef>
#include <exception>
#include <expected>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <format>
#include <future>
#include <initializer_list>
#include <memory>
#include <source_location>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace srs
{
    internal::AppExitHelper::~AppExitHelper() noexcept { app_->action_after_destructor(); }

    void App::init_spdlog()
    {
        auto log_file = common::get_default_log_path();

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%H:%M:%S][%^%=6l%$][%t] %v");
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file.string(), common::file_max_size, common::rotating_file_nums, false);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S][%l][%t] %v");

        auto logger =
            std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{ console_sink, file_sink });

        console_sink->set_level(spdlog::level::off);
        logger->info("");
        logger->info("{}", common::file_logger_new_instance_str);
        logger->info("");
        console_sink->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        // spdlog::set_pattern("[%H:%M:%S][%^%=6l%$][%t] %v");
        spdlog::info("Welcome to SRS Application");
    }

    App::App()
        : io_work_guard_{ asio::make_work_guard(io_context_) }
        , fec_strand_{ asio::make_strand(io_context_.get_executor()) }
    {
        init_spdlog();
    }

    void App::action_after_destructor()
    {
        auto _ = ExitLogger{};
        io_work_guard_.reset();
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
            working_thread_.join();
            spdlog::debug("Application: All tasks in main io_context are finished.");
        }
    }

    App::~App() noexcept
    {
        auto _ = ExitLogger{};
        auto err = boost::system::error_code{};
        signal_set_.cancel(err);
        signal_set_.clear(err);
    }

    void App::init()
    {
        const auto _ = ExitLogger{};
        workflow_handler_ = std::make_unique<workflow::Handler>(this);
        workflow_handler_->set_n_pipelines(config_.output_split);
        workflow_handler_->set_output_filenames(config_.output_filenames);

        set_remote_fec_endpoints();
        check_port_number_validity();
        signal_set_.async_wait(
            [this](const boost::system::error_code& error, auto)
            {
                spdlog::info("Application: Calling SIGINT from monitoring thread");
                if (error == asio::error::operation_aborted)
                {
                    return;
                }
                exit_and_switch_off();
            });
        auto current_loc = std::source_location::current();
        auto monitoring_action = [this, current_loc]()
        {
            try
            {
                auto _ = ExitLogger{ "io_context join", current_loc };
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
        // sequentially waiting
        for (auto& socket : data_sockets_)
        {
            if (socket == nullptr)
            {
                continue;
            }
            socket->cancel();
            auto status = socket->wait_for_listen_finish(common::DEFAULT_STATUS_WAITING_TIME_SECONDS);
            if (status.has_value() and status.value() == std::future_status::timeout)
            {
                spdlog::warn("Application: TIMEOUT during waiting for the closing of input data stream.");
            }
        }
    }

    // This will be called by Ctrl-C interrupt
    void App::exit_and_switch_off()
    {
        fmt::println("");
        spdlog::debug("Application: exiting ...");
        spdlog::default_logger()->flush();
        spdlog::flush_every(std::chrono::seconds{ 1 });
        auto _ = ExitLogger{};
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
        config_.output_filenames = filenames;
        config_.output_split = n_lines;
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
        return connection::SpecialSocket::create<connection::FecCommandSocket>(config_.fec_control_local_port,
                                                                               io_context_)
            .transform(
                [this, connection_name](const std::shared_ptr<connection::FecCommandSocket>& socket)
                {
                    for (const auto& remote_endpoint : remote_fec_endpoints_)
                    {
                        auto fec_connection = std::make_shared<T>(connection_name);
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

    auto App::check_port_number_validity() -> bool
    {
        const auto& data_ports = config_.fec_data_receive_ports;

        if (auto port = std::ranges::find(data_ports, config_.fec_control_local_port); port != data_ports.end())
        {
            throw std::runtime_error(
                std::format("Application: port number {} used for input data stream is not allowed to be the same "
                            "port number used for the local control port {}.",
                            *port,
                            config_.fec_control_local_port));
        }
        return true;
    }

    void App::read_data(bool /*is_non_stop*/)
    {
        spdlog::info("Application: Starting input data stream from local port number(s): [{}]...",
                     fmt::join(config_.fec_data_receive_ports, ", "));
        for (const auto port_num : config_.fec_data_receive_ports)
        {
            auto status =
                connection::SpecialSocket::create<connection::DataSocket>(
                    port_num, io_context_, workflow_handler_.get())
                    .transform(
                        [this](auto socket)
                        {
                            spdlog::debug("data stream is using the buffer size: {}", config_.data_buffer_size);
                            socket->set_buffer_size(config_.data_buffer_size);
                            data_sockets_.push_back(std::move(socket));
                        });

            if (not status.has_value())
            {
                spdlog::critical("Application: Cannot establish the connection for the input data stream because the "
                                 "local port number "
                                 "{} is not available.",
                                 port_num);
            }
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
