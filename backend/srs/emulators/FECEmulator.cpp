#include "FECEmulator.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/Connections.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/emulators/RandomizeStruct.hpp"
#include "srs/emulators/World.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/writers/UDPWriter.hpp"
#include <algorithm>
#include <asio/any_io_executor.hpp>
#include <asio/as_tuple.hpp>
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/detached.hpp>
#include <asio/impl/co_spawn.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include <asio/ip/udp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/use_future.hpp>
#include <chrono>
#include <cstddef>
#include <expected>
#include <fmt/ranges.h>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <random>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace srs::emulator
{
    namespace
    {
        auto get_msg_from_receive_type(FECEmulator::ReceiveType rec_type) -> std::string
        {
            using enum FECEmulator::ReceiveType;
            switch (rec_type)
            {
                case invalid:
                    return {};
                case acq_on:
                {
                    auto acq_on_connect = connection::Starter{};
                    return std::string{ acq_on_connect.get_response_msg() };
                }
                case acq_off:
                    auto acq_off_connect = connection::Stopper{};
                    return std::string{ acq_off_connect.get_response_msg() };
            }
            return {};
        }
        auto get_send_msg_from_receive_type(FECEmulator::ReceiveType rec_type) -> std::string
        {
            auto buffer = process::SerializableMsgBuffer{};
            using enum FECEmulator::ReceiveType;
            switch (rec_type)
            {
                case invalid:
                    return {};
                case acq_on:
                {
                    auto acq_on_connect = connection::Starter{};
                    const auto& send_suffix = acq_on_connect.get_send_suffix();
                    buffer.serialize(send_suffix);
                    return std::string{ buffer.data() };
                }
                case acq_off:
                    auto acq_off_connect = connection::Stopper{};
                    const auto& send_suffix = acq_off_connect.get_send_suffix();
                    buffer.serialize(send_suffix);
                    return std::string{ buffer.data() };
            }
            return {};
        }

        auto check_receive_msg_type(std::string_view msg) -> FECEmulator::ReceiveType
        {
            using enum FECEmulator::ReceiveType;
            static const auto msg_map = std::map<std::string, FECEmulator::ReceiveType>{
                { get_send_msg_from_receive_type(acq_on), acq_on },
                { get_send_msg_from_receive_type(acq_off), acq_off },
            };
            auto msg_iter = std::ranges::find_if(
                msg_map, [msg](const auto& suffix_type) -> bool { return msg.ends_with(suffix_type.first); });
            // auto msg_iter = msg_map.find_if(std::string{ msg }, );
            spdlog::trace("msg_map available options {}",
                          fmt::join(msg_map | std::views::transform(
                                                  [](const auto& str_type) -> std::string
                                                  {
                                                      return fmt::format("{}: {:02x}",
                                                                         magic_enum::enum_name(str_type.second),
                                                                         fmt::join(str_type.first, " "));
                                                  }),
                                    "\n"));
            if (msg_iter == msg_map.end())
            {
                return invalid;
            }
            return msg_iter->second;
        }
    } // namespace

    FECEmulator::FECEmulator(const Config& config, World& world, IOContextType& io_context)
        : io_context_{ &io_context }
        , config_{ config }
        , udp_socket_{ io_context,
                       asio::ip::udp::endpoint{ asio::ip::make_address_v4(config_.ip),
                                                static_cast<asio::ip::port_type>(config_.port) } }
        , struct_randomizer_{ Randomizer::Config{ .n_hits = config.n_hits, .n_markers = config.n_markers } }
        , world_{ &world }
    {
        spdlog::debug("Emulator: binds to a local udp socket with port number: {} and ip address: {}",
                      udp_socket_.local_endpoint().port(),
                      udp_socket_.local_endpoint().address().to_string());
        data_sender_wait_timer_->expires_at(std::chrono::system_clock::time_point::max());
    }

    void FECEmulator::wait_for_connection()
    {
        spdlog::info("FEC with {} is active.", udp_socket_.local_endpoint());

        if (config_.is_sent_data_only)
        {
            do_if_acq_on(io_context_->get_executor());
        }
        else
        {
            // NOLINTBEGIN (clang-analyzer-core.CallAndMessage)
            asio::co_spawn(*io_context_, listen_coro(), asio::detached);
            // NOLINTEND (clang-analyzer-core.CallAndMessage)
        }
    }

    void FECEmulator::do_if_acq_on(asio::any_io_executor executor)
    {
        spdlog::info("FEC with {} is switched on. Start sending data.", udp_socket_.local_endpoint());
        is_switched_off_.store(false);
        asio::co_spawn(executor, start_send_data(), asio::detached);
        // spdlog::trace("Server: start_send_data spawned!");
    }

    void FECEmulator::do_if_acq_off()
    {
        spdlog::info("FEC with {} is switched off. Stop sending data.", udp_socket_.local_endpoint());
        is_switched_off_.store(true);
    }

    auto FECEmulator::send_response(connection::udp::endpoint endpoint, ReceiveType result_type)
        -> asio::awaitable<void>
    {
        const auto _ = ExitLogger{ config_.ip };
        auto msg = get_msg_from_receive_type(result_type);
        [[maybe_unused]] const auto size =
            co_await udp_socket_.async_send_to(asio::buffer(msg), endpoint, asio::use_awaitable);
        spdlog::trace("SRS server: sent the data {:02x} to the remote point {}", fmt::join(msg, " "), endpoint);
    }

    auto FECEmulator::listen_coro() -> asio::awaitable<void>
    {
        auto msg_buffer = std::vector<char>{};
        msg_buffer.resize(common::SMALL_READ_MSG_BUFFER_SIZE);
        auto executor = co_await asio::this_coro::executor;
        for (;;)
        {
            spdlog::trace("Emulator: listening on the local port {}", udp_socket_.local_endpoint());
            if (is_switched_off_.load())
            {
                spdlog::info("FEC with {} is ready for connections.", udp_socket_.local_endpoint());
            }
            auto remote_endpoint = asio::ip::udp::endpoint{};
            auto msg_size =
                co_await udp_socket_.async_receive_from(asio::buffer(msg_buffer), remote_endpoint, asio::use_awaitable);
            auto msg = std::string_view{ msg_buffer.data(), msg_size };
            auto result = check_receive_msg_type(msg);
            spdlog::trace("Emulator: Request type {:?} received with the msg: {:02x} in ip: {}",
                          magic_enum::enum_name(result),
                          fmt::join(msg, " "),
                          config_.ip);
            asio::co_spawn(*io_context_, send_response(std::move(remote_endpoint), result), asio::detached);

            using enum ReceiveType;
            switch (result)
            {
                case invalid:
                    break;
                case acq_on:
                    do_if_acq_on(executor);
                    break;
                case acq_off:
                    do_if_acq_off();
                    if (not world_->get_config().non_stop)
                    {
                        co_return;
                    }
                    break;
            }
        }
        spdlog::info("Existing emulator");
    }

    auto FECEmulator::construct_binary_data() -> std::expected<std::size_t, std::string_view>
    {
        const auto frame_counter = world_->request_frame_counter();

        struct_randomizer_.randomize_data_struct(data_struct_, { .frame_counter = frame_counter, .fec_id = fec_id_ });
        return struct_serializer_.convert(&data_struct_);
    }

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif
    auto FECEmulator::start_send_data() -> asio::awaitable<void>
    {
        auto device = std::random_device{};
        auto rand_gen = std::mt19937{ device() };
        auto distrib = std::uniform_int_distribution<std::chrono::nanoseconds::rep>{ std::chrono::nanoseconds::rep{},
                                                                                     delay_time_ };
        const auto _ = ExitLogger{ config_.ip };
        auto total_size = std::size_t{ 0 };
        auto connection = connection::UDPWriterConnection{ *io_context_,
                                                           asio::ip::udp::endpoint{ asio::ip::make_address("127.0.0.1"),
                                                                                    static_cast<asio::ip::port_type>(
                                                                                        config_.remote_data_port) } };

        spdlog::debug(
            "Emulator: Starting to send data to port {} from ip {} ...", config_.remote_data_port, config_.ip);
        data_sending_control_.expires_after(std::chrono::seconds{ 1 });
        co_await data_sending_control_.async_wait(asio::use_awaitable);

        auto executor = co_await asio::this_coro::executor;

        using asio::experimental::awaitable_operators::operator||;
        auto cancel_timer_ = asio::steady_timer{ executor };
        cancel_timer_.expires_at(asio::steady_timer::time_point::max());

        while (not is_switched_off_.load())
        {
            const auto time_ns = distrib(rand_gen);
            data_sending_control_.expires_after(std::chrono::nanoseconds{ time_ns });
            co_await data_sending_control_.async_wait(asio::use_awaitable);

            auto res = construct_binary_data();
            if (not res)
            {
                spdlog::error("{}", res.error());
                break;
            }
            auto read_str = struct_serializer_();

            auto send_size = connection.send_sync_message(read_str);
            total_size += send_size;
            ++n_frames_sent_;

            data_sending_control_.expires_after(std::chrono::nanoseconds{ delay_time_ - time_ns });
            auto res_timer = co_await (data_sending_control_.async_wait(asio::use_awaitable) ||
                                       cancel_timer_.async_wait(asio::use_awaitable));
            if (res_timer.index() == 1)
            {
                spdlog::debug("Runtime has reached. Cancelling FEC data sending.");
                break;
            }
        }
        connection.close();
        spdlog::debug(
            "Emulator: reaching the end of send_data coroutine. Sent number of frames: {} and total data size: "
            "{} from {}",
            n_frames_sent_,
            total_size,
            udp_socket_.local_endpoint());
        data_sender_wait_timer_->cancel();
        data_sender_wait_timer_->expires_after(std::chrono::seconds{ 0 });
        cancel_timer_.cancel();
        cancel_timer_.expires_after(std::chrono::seconds{ 0 });
    }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

} // namespace srs::emulator
