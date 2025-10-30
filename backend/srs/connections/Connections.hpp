#pragma once

#include "ConnectionBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
// #include "srs/utils/CommonAlias.hpp"
// #include "srs/utils/CommonDefinitions.hpp"
#include <gsl/gsl-lite.hpp>
#include <memory>
// #include <span>
#include <spdlog/spdlog.h>
#include <vector>

namespace srs::workflow
{
    class Handler;
}

namespace srs::connection
{
    class FecSwitchSocket;
    class Starter : public Base
    {
      public:
        // explicit Starter(const Info& info);
        explicit Starter(const Config& config);
        Starter();
        void close();
        void send_message_from(std::shared_ptr<FecSwitchSocket> socket);
        // void acq_on();
        void on_fail()
        {
            set_connection_bad();
            const auto& endpoint = get_remote_endpoint();
            spdlog::critical("Cannot establish a connection to the FEC with the IP: \"{}:{}\"!",
                             endpoint.address().to_string(),
                             endpoint.port());
        }
        auto get_send_suffix() const -> const auto& { return send_suffix_; }

      private:
        std::vector<CommunicateEntryType> send_suffix_ = { 0, 15, 1 };
    };

    class Stopper : public Base
    {
      public:
        /**
         * \brief Deleted copy constructor (rule of 5).
         */
        Stopper(const Stopper&) = delete;

        /**
         * \brief Deleted move constructor (rule of 5).
         */
        Stopper(Stopper&&) = delete;

        /**
         * \brief Deleted copy assignment operator (rule of 5).
         */
        Stopper& operator=(const Stopper&) = delete;

        /**
         * \brief Deleted move assignment operator (rule of 5).
         */
        Stopper& operator=(Stopper&&) = delete;

        /**
         * \brief Constructor for Stopper connection class
         *
         * @param config connection configuration
         */
        explicit Stopper(const Config& config);

        Stopper();

        /**
         * \brief Destructor for Stopper connection class
         *
         * The destructor change the Status::is_acq_off to be true
         * @see Status
         */
        ~Stopper() = default;

        /**
         * \brief called if an error occurs
         */
        void on_fail()
        {
            spdlog::debug("on_fail of stopper is called");
            set_connection_bad();
        }

        /**
         * \brief Turn off the data acquisition from SRS system
         *
         * This is the primary execution from the Stopper class. It first checks if the Status::is_acq_on from the App
         * is true. If the status is still false after 4 seconds, an exception will be **thrown**. If the status is
         * true, member function connection::Base::communicate would be called.
         * @see connection::Base::communicate
         */
        // void acq_off();
        void send_message_from(std::shared_ptr<FecSwitchSocket> socket);
        // void close() {}
        auto get_send_suffix() const -> const auto& { return send_suffix_; }

      private:
        std::vector<CommunicateEntryType> send_suffix_ = { 0, 15, 0 };
    };

    /**
     * @class DataReader
     * @brief Connection for reading data stream from FEC devices
     *
     */
    // class DataReader : public Base
    // {
    //   public:
    //     DataReader(const Info& info, workflow::Handler* processor)
    //         : Base(info, "DataReader", common::LARGE_READ_MSG_BUFFER_SIZE)
    //         , workflow_handler_{ processor }
    //     {
    //         set_timeout_seconds(1);
    //         set_continuous();
    //     }

    //     DataReader(const DataReader&) = delete;
    //     DataReader(DataReader&&) = delete;
    //     DataReader& operator=(const DataReader&) = delete;
    //     DataReader& operator=(DataReader&&) = delete;
    //     ~DataReader() = default;

    //     void start(bool is_non_stop = true)
    //     {
    //         set_socket(new_shared_socket(get_local_port_number()));
    //         // const auto& is_on_exit = get_app().get_status().is_on_exit;
    //         // if (not is_on_exit.load())
    //         // {
    //         //     get_app().set_status_is_reading(true);
    //         //     spdlog::info("UDP data reading has been started");
    //         //     listen(is_non_stop);
    //         // }
    //         // else
    //         // {
    //         //     spdlog::debug("Program is already on exit!");
    //         // }
    //     }
    //     void close();

    //     void read_data_handle(std::span<BufferElementType> read_data);

    //   private:
    //     gsl::not_null<workflow::Handler*> workflow_handler_;
    // };

} // namespace srs::connection
