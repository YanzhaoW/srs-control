#pragma once

#include "ConnectionBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include <array>
#include <gsl/gsl-lite.hpp>
#include <span>
#include <spdlog/spdlog.h>
#include <string_view>

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
        explicit Starter(std::string_view name);
        Starter();
        void close();
        static auto get_send_suffix() -> const auto& { return send_suffix_; }
        static auto get_suffix() -> std::span<const CommunicateEntryType>
        {
            return std::span{ send_suffix_.cbegin(), send_suffix_.cend() };
        }

      private:
        static constexpr std::array<CommunicateEntryType, 3> send_suffix_ = { 0, 15, 1 };
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

        static auto get_send_suffix() -> const auto& { return send_suffix_; }
        static auto get_suffix() -> std::span<const CommunicateEntryType>
        {
            return std::span{ send_suffix_.cbegin(), send_suffix_.cend() };
        }

      private:
        static constexpr std::array<CommunicateEntryType, 3> send_suffix_ = { 0, 15, 0 };
    };
} // namespace srs::connection
