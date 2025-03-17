#pragma once

#include <thread>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>

// #include <srs/devices/Fec.hpp>
#include <srs/devices/Configuration.hpp>
#include <srs/utils/AppStatus.hpp>
#include <srs/utils/CommonAlias.hpp>

namespace srs
{
    namespace workflow
    {
        class Handler;
    } // namespace workflow

    namespace connection
    {
        class DataReader;
    }

    class App;

    class AppExitHelper
    {
      public:
        explicit AppExitHelper(App* app)
            : app_{ app }
        {
        }

        AppExitHelper(const AppExitHelper&) = default;
        AppExitHelper(AppExitHelper&&) = delete;
        AppExitHelper& operator=(const AppExitHelper&) = default;
        AppExitHelper& operator=(AppExitHelper&&) = delete;
        ~AppExitHelper() noexcept;

      private:
        App* app_;
    };

    class App
    {
      public:
        App();

        App(const App&) = delete;
        App(App&&) = delete;
        App& operator=(const App&) = delete;
        App& operator=(App&&) = delete;
        ~App() noexcept;

        // public APIs
        void init();
        void configure_fec() {}
        void switch_on();
        void switch_off();
        void read_data(bool is_non_stop = true);

        void notify_status_change() { status_.status_change.notify_all(); }
        void start_workflow(bool is_blocking = true);
        void wait_for_finish();
        auto wait_for_status(auto&& condition,
                             std::chrono::seconds time_duration = common::DEFAULT_STATUS_WAITING_TIME_SECONDS) -> bool
        {
            return status_.wait_for_status(std::forward<decltype(condition)>(condition), time_duration);
        }

        // setters:
        void set_remote_endpoint(std::string_view remote_ip, int port_number);
        void set_fec_data_receiv_port(int port_num) { configurations_.fec_data_receive_port = port_num; }
        void set_status_acq_on(bool val = true) { status_.is_acq_on.store(val); }
        void set_status_acq_off(bool val = true) { status_.is_acq_off.store(val); }
        void set_status_is_reading(bool val = true) { status_.is_reading.store(val); }
        void set_print_mode(common::DataPrintMode mode);
        void set_output_filenames(const std::vector<std::string>& filenames);
        void set_error_string(std::string_view err_msg) { error_string_ = err_msg; }

        // getters:
        [[nodiscard]] auto get_channel_address() const -> uint16_t { return channel_address_; }
        // [[nodiscard]] auto get_fec_config() const -> const auto& { return fec_config_; }
        [[nodiscard]] auto get_status() const -> const auto& { return status_; }
        [[nodiscard]] auto get_io_context() -> auto& { return io_context_; }
        auto get_data_reader() -> connection::DataReader* { return data_reader_.get(); }
        [[nodiscard]] auto get_error_string() const -> const std::string& { return error_string_; }
        [[nodiscard]] auto get_workflow_handler() const -> const auto& { return *workflow_handler_; };

        // called by ExitHelper
        void end_of_work();

      private:
        using udp = asio::ip::udp;

        Status status_;
        uint16_t channel_address_ = common::DEFAULT_CHANNEL_ADDRE;
        Config configurations_;
        std::string error_string_;

        // Destructors are called in the inversed order
        io_context_type io_context_{ 4 };
        asio::executor_work_guard<io_context_type::executor_type> io_work_guard_;
        udp::endpoint remote_endpoint_;
        asio::signal_set signal_set_{ io_context_, SIGINT, SIGTERM };
        std::jthread working_thread_;
        AppExitHelper exit_helper_{ this };
        std::unique_ptr<workflow::Handler> workflow_handler_;
        std::shared_ptr<connection::DataReader> data_reader_;

        void exit();
        void wait_for_reading_finish();
    };

} // namespace srs
