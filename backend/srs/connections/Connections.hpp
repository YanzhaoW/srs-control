#pragma once

#include "ConnectionBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

namespace srs::workflow
{
    class Handler;
}

namespace srs::connection
{
    class FecCommandSocket;
    class Starter : public Base
    {
      public:
        // explicit Starter(const Info& info);
        explicit Starter(std::string_view name);
        Starter();
        void close();
        void send_message_from(std::shared_ptr<FecCommandSocket> socket);
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
         * @param name Name of the command
         */
        explicit Stopper(std::string_view name);

        Stopper();

        /**
         * \brief Destructor for Stopper connection class
         *
         * The destructor change the Status::is_acq_off to be true
         * @see Status
         */
        ~Stopper() = default;

        /**
         * \brief Turn off the data acquisition from SRS system
         *
         * This is the primary execution from the Stopper class. It first checks if the Status::is_acq_on from the App
         * is true. If the status is still false after 4 seconds, an exception will be **thrown**. If the status is
         * true, member function connection::Base::communicate would be called.
         * @see connection::Base::communicate
         */
        // void acq_off();
        void send_message_from(std::shared_ptr<FecCommandSocket> socket);
        // void close() {}
        auto get_send_suffix() const -> const auto& { return send_suffix_; }

      private:
        std::vector<CommunicateEntryType> send_suffix_ = { 0, 15, 0 };
    };
} // namespace srs::connection
