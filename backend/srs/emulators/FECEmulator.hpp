#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/converters/StructSerializer.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/emulators/RandomizeStruct.hpp"
#include "srs/utils/CommonAlias.hpp" // IWYU pragma: keep
#include "srs/utils/CommonDefinitions.hpp"
#include <asio.hpp>
#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/deferred.hpp>
#include <asio/impl/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include <asio/ip/udp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/system_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/uses_executor.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace srs::emulator
{
    class World;
    class FECEmulator
    {
      public:
        using IOContextType = asio::thread_pool;
        struct Config
        {

            std::string ip;
            int port = common::DEFAULT_SRS_CONTROL_PORT;
            bool is_sent_data_only = false;
            int remote_data_port = common::FEC_DAQ_RECEIVE_PORT.front();
            common::MinMax<std::size_t> n_hits{ 0, 10 };
            common::MinMax<std::size_t> n_markers{ 0, 10 };
        };

        enum class ReceiveType : uint8_t
        {
            invalid,
            acq_on,
            acq_off,
        };

        explicit FECEmulator(const Config& config, World& world, IOContextType& io_context);

        void set_delay_time(std::size_t time) { delay_time_ = static_cast<std::chrono::nanoseconds::rep>(time); }
        void wait_for_connection();
        void set_fec_id(uint8_t fec_id) { fec_id_ = fec_id; }

        auto get_config() const -> const auto& { return config_; }

        auto get_n_frames_sent() const -> const auto& { return n_frames_sent_; }

      private:
        std::chrono::nanoseconds::rep delay_time_ = 3;
        std::atomic<bool> is_switched_off_ = true;
        uint8_t fec_id_{};
        gsl::not_null<IOContextType*> io_context_;
        std::string source_filename_;
        std::size_t n_frames_sent_ = 0;
        Config config_;
        asio::ip::udp::socket udp_socket_;
        std::shared_ptr<asio::system_timer> data_sender_wait_timer_ =
            std::make_shared<asio::system_timer>(*io_context_);
        asio::steady_timer data_sending_control_{ *io_context_ };
        process::StructSerializer struct_serializer_{};
        StructData data_struct_{};
        Randomizer struct_randomizer_;
        gsl::not_null<World*> world_;

        auto start_send_data() -> asio::awaitable<void>;

        void do_if_acq_on(asio::any_io_executor executor);

        void do_if_acq_off();

        void listen_on_communications();
        // auto cancel_coroutine() -> cobalt::promise<void>;

        auto listen_coro() -> asio::awaitable<void>;
        auto send_response(connection::udp::endpoint endpoint, ReceiveType result_type) -> asio::awaitable<void>;

        auto construct_binary_data() -> std::expected<std::size_t, std::string_view>;
    };
} // namespace srs::emulator
