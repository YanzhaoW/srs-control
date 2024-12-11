#pragma once

#include "ConnectionBase.hpp"

namespace srs::workflow
{
    class Handler;
}

namespace srs::connection
{
    class Starter : public Base<>
    {
      public:
        explicit Starter(const Info& info)
            : Base(info, "Starter")
        {
        }

        void close();
        void acq_on()
        {
            const auto data = std::vector<CommunicateEntryType>{ 0, 15, 1 };
            communicate(data, common::NULL_ADDRESS);
        }
        void on_fail()
        {
            const auto& endpoint = get_remote_endpoint();
            spdlog::critical(
                "Cannot start the system: No connection to {}:{}!", endpoint.address().to_string(), endpoint.port());
        }
    };

    class Stopper : public Base<>
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
         * @param info connection information
         */
        explicit Stopper(const Info& info)
            : Base(info, "Stopper")
        {
        }

        /**
         * \brief Destructor for Stopper connection class
         *
         * The destructor change the Status::is_acq_off to be true
         * @see Status
         */
        ~Stopper()
        {
            spdlog::info("SRS system is turned off");
            get_app().set_status_acq_off();
        }

        /**
         * \brief called if an error occurs
         */
        static void on_fail() { spdlog::debug("on_fail of stopper is called"); }

        /**
         * \brief Turn off the data acquisition from SRS system
         *
         * This is the primary execution from the Stopper class. It first checks if the Status::is_acq_on from the App
         * is true. If the status is still false after 4 seconds, an exception will be **thrown**. If the status is
         * true, member function connection::Base::communicate would be called.
         * @see connection::Base::communicate
         */
        void acq_off();
        // void close() {}
    };

    class DataReader : public Base<common::LARGE_READ_MSG_BUFFER_SIZE>
    {
      public:
        DataReader(const Info& info, workflow::Handler* processor)
            : workflow_handler_{ processor }
            , Base(info, "DataReader")
        {
            set_timeout_seconds(1);
            set_continuous();
        }

        DataReader(const DataReader&) = delete;
        DataReader(DataReader&&) = delete;
        DataReader& operator=(const DataReader&) = delete;
        DataReader& operator=(DataReader&&) = delete;
        ~DataReader() = default;

        void start(bool is_non_stop = true)
        {
            set_socket(new_shared_socket(get_local_port_number()));
            const auto& is_on_exit = get_app().get_status().is_on_exit;
            if (not is_on_exit.load())
            {
                get_app().set_status_is_reading(true);
                listen(is_non_stop);
            }
            else
            {
                spdlog::debug("Program is alreay on exit!");
            }
        }
        void close();

        void read_data_handle(std::span<BufferElementType> read_data);

      private:
        gsl::not_null<workflow::Handler*> workflow_handler_;
    };

} // namespace srs::connection
