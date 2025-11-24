#pragma once

#include "srs/devices/Configuration.hpp"
#include "srs/utils/AppStatus.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

// #include <srs/devices/Fec.hpp>
namespace srs
{
    namespace workflow
    {
        class Handler;
    } // namespace workflow

    namespace connection
    {
        class DataReader;
        class DataSocket;
    } // namespace connection

    class App;

    namespace internal
    {
        /**
         * @class AppExitHelper
         * @brief An internal exit helper for App class.
         */
        class AppExitHelper
        {
          public:
            /**
             * @brief Constructor with pointer to srs::App instance
             */
            explicit AppExitHelper(App* app)
                : app_{ app }
            {
            }

            AppExitHelper(const AppExitHelper&) = default;
            AppExitHelper(AppExitHelper&&) = delete;
            AppExitHelper& operator=(const AppExitHelper&) = default;
            AppExitHelper& operator=(AppExitHelper&&) = delete;
            /**
             * @brief Destructor calling srs::App::end_of_work method.
             */
            ~AppExitHelper() noexcept;

          private:
            App* app_;
        };

    } // namespace internal

    /**
     * @class App
     * @brief The primary interface class of SRS-Control.
     *
     * Application class should be instantiated only once during the whole program. Public setter methods should be
     * called before calling the #init() method.
     */

    class App
    {
      public:
        /**
         * @brief Constructor of the App class.
         *
         * The constructor contains following actions:
         * 1. Instantiation of the work guard, which make sure the io_context_ keep accepting new tasks.
         * 2. Instantiation of strand from the #io_context_, which synchronizes the communications among the FECs.
         * 3. Set the logging format.
         * 4. Instantiation (memory allocation) of workflow_handler_.
         */
        App();

        App(const App&) = delete;
        App(App&&) = delete;
        App& operator=(const App&) = delete;
        App& operator=(App&&) = delete;

        /**
         * @brief Destructor of the App class.
         *
         * This destructor is called before the destruction of each members. The destructor contains following actions:
         * 1. Cancel the signal_set_.
         * 2. Wait for the Status::is_reading to be false, which will be unset from connection::DataReader::close(). The
         * waiting time is set to be common::DEFAULT_STATUS_WAITING_TIME_SECONDS.
         * 3. Requesting acquisition off to all FEC devices.
         * 4. Set the Status::is_acq_on to be true.
         *
         * The exit process of App class also contains the destruction of its member variables. The following order must
         * be kept:
         *
         * 1. Destruction of connection::DataReader member.
         * 2. Destruction of workflow::Handler member.
         * 3. Destruction of internal::AppExitHelper member, which calls end_of_work method.
         * 4. Destruction of working_thread_ (std::jthread) by **waiting** for the thread to finish.
         * 5. Destruction other members. Orders are not important.
         * 6. Destruction of io_context member. This must be done in the very end.
         *
         * @sa internal::AppExitHelper
         */
        ~App() noexcept;

        /**
         * @brief Initialization of internal members.
         */
        void init();

        void configure_fec() {}

        /**
         * @brief Establish the communications to the available FECs to start the data acquisition.
         */
        void switch_on();

        /**
         * @brief Establish the communications to the available FECs to stop the data acquisition.
         */
        void switch_off();

        /**
         * @brief Start reading the input data stream from the port specified by srs::Config::fec_data_receive_port. If
         * \a is_non_stop is true, the reading process will not be stopped until an interrupt happens, such as pressing
         * "Ctrl-C" from users. If \a is_non_stop is false, the reading process will be stopped after one frame of data
         * is read.
         *
         * @param is_non_stop Flag to set non-stop mode.
         */
        void read_data(bool is_non_stop = true);

        /**
         * @brief Start data analysis workflow, triggering data conversions.
         *
         * This function call is executed in the main thread.
         *
         */
        void start_workflow();

        /**
         * @brief Manually wait for the working thread to finish.
         *
         * This method is called automatically in the destructor and shouldn't be called if not necessary. It blocks the
         * program until the working thread exits.
         */
        void wait_for_finish();

        void wait_for_workflow() { workflow_thread_.join(); }

        // setters:
        /**
         * @brief Set the local listen port number for the communications to FEC devices
         */
        void set_fec_data_receiv_port(int port_num) { config_.fec_data_receive_port = port_num; }

        /**
         * @brief Set the print mode.
         */
        void set_print_mode(common::DataPrintMode mode);

        /**
         * @brief Set the output filenames.
         */
        void set_output_filenames(const std::vector<std::string>& filenames, std::size_t n_lines = 1);

        /**
         * @brief Set the error messages (internal usage).
         */
        void set_error_string(std::string_view err_msg) { error_string_ = err_msg; }

        /**
         * @brief Set the configuration values.
         */
        void set_options(Config options) { config_ = std::move(options); }

        void disable_switch_off(bool is_disabled = true) { is_switch_off_disabled_ = is_disabled; }

        void exit_and_switch_off();

        // internal usage
        // TODO: Refactor the code to not expose those methods for the internal usage

        auto wait_for_status(auto&& condition,
                             std::chrono::seconds time_duration = common::DEFAULT_STATUS_WAITING_TIME_SECONDS) -> bool
        {
            return status_.wait_for_status(std::forward<decltype(condition)>(condition), time_duration);
        }
        void notify_status_change() { status_.status_change.notify_all(); }
        void set_status_acq_on(bool val = true) { status_.is_acq_on.store(val); }
        void set_status_acq_off(bool val = true) { status_.is_acq_off.store(val); }
        void set_status_is_reading(bool val = true) { status_.is_reading.store(val); }
        // getters:
        [[nodiscard]] auto get_channel_address() const -> uint16_t { return channel_address_; }
        // [[nodiscard]] auto get_fec_config() const -> const auto& { return fec_config_; }
        [[nodiscard]] auto get_status() const -> const auto& { return status_; }
        [[nodiscard]] auto get_io_context() -> auto& { return io_context_; }
        [[nodiscard]] auto get_fec_strand() -> auto& { return fec_strand_; }
        auto get_data_reader_socket() -> connection::DataSocket* { return data_socket_.get(); }
        [[nodiscard]] auto get_error_string() const -> const std::string& { return error_string_; }
        [[nodiscard]] auto get_workflow_handler() const -> const auto& { return *workflow_handler_; };
        [[nodiscard]] auto get_config() const -> const auto& { return config_; }
        [[nodiscard]] auto get_config_ref() -> auto& { return config_; }

        // called by ExitHelper
        void action_after_destructor();

      private:
        using udp = asio::ip::udp;
        using SwitchFutureType = std::expected<std::future<void>, boost::system::error_code>;
        using SwitchFutureStatusType = std::expected<std::future_status, boost::system::error_code>;

        Status status_;
        bool is_switch_off_disabled_ = false;
        uint16_t channel_address_ = common::DEFAULT_CHANNEL_ADDRE;
        Config config_;
        std::string error_string_;

        // Destructors are called in the inverse order

        /** @brief Asio io_context that manages the task scheduling and network IO.
         */
        io_context_type io_context_{ 4 };

        /** @brief Asio io_context work guard.
         */
        asio::executor_work_guard<io_context_type::executor_type> io_work_guard_;

        /** @brief Remote endpoints of FEC devices.
         */
        std::vector<udp::endpoint> remote_fec_endpoints_;

        /** @brief User signal handler for interrupts.
         */
        asio::signal_set signal_set_{ io_context_, SIGINT, SIGTERM };

        /** @brief FEC communication strand for synchronous communications.
         */
        asio::strand<io_context_type::executor_type> fec_strand_;

        /** @brief Main working thread.
         */
        std::jthread working_thread_;

        /** @brief Main thread to run workflow.
         */
        std::jthread workflow_thread_;

        /**
         * @brief Exit helper for App class. This is called after calling the destructor.
         *
         */
        internal::AppExitHelper exit_helper_{ this };

        /** @brief The handler to the analysis working flow.
         */
        std::unique_ptr<workflow::Handler> workflow_handler_;

        /** @brief Communication to the main input data stream.
         */
        // std::shared_ptr<connection::DataReader> data_reader_;

        std::shared_ptr<connection::DataSocket> data_socket_;

        SwitchFutureType switch_on_future_;

        SwitchFutureType switch_off_future_;

        void wait_for_reading_finish();
        void set_remote_fec_endpoints();
        void add_remote_fec_endpoint(std::string_view remote_ip, int port_number);
        static auto wait_for_switch_action(const SwitchFutureType& switch_future) -> SwitchFutureStatusType;

        template <typename T>
        auto switch_FECs(std::string_view connection_name) -> SwitchFutureType;
    };

} // namespace srs
