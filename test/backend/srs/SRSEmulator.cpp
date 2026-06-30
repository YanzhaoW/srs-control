#include "SRSEmulator.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/Connections.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/writers/UDPWriter.hpp"
#include <algorithm>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <chrono>
#include <cstddef>
#include <fmt/ranges.h>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace srs::test
{
    namespace
    {
        auto get_msg_from_receive_type(SRSEmulator::ReceiveType rec_type) -> std::string
        {
            using enum SRSEmulator::ReceiveType;
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
        auto get_send_msg_from_receive_type(SRSEmulator::ReceiveType rec_type) -> std::string
        {
            auto buffer = process::SerializableMsgBuffer{};
            using enum SRSEmulator::ReceiveType;
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

        auto check_receive_msg_type(std::string_view msg) -> SRSEmulator::ReceiveType
        {
            using enum SRSEmulator::ReceiveType;
            static const auto msg_map = std::map<std::string, SRSEmulator::ReceiveType>{
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

    SRSEmulator::SRSEmulator(const Config& config, IOContextType& io_context)
        : io_context_{ &io_context }
        , config_{ config }
        , udp_socket_{ io_context,
                       asio::ip::udp::endpoint{ asio::ip::make_address_v4(config_.ip),
                                                static_cast<asio::ip::port_type>(config_.listen_port) } }
        , frame_reader_{ std::string{ config.filename } }
    {
        spdlog::debug("Emulator: binds to a local udp socket with port number: {} and ip address: {}",
                      udp_socket_.local_endpoint().port(),
                      udp_socket_.local_endpoint().address().to_string());
        data_sender_wait_timer_->expires_at(std::chrono::system_clock::time_point::max());
    }

    void SRSEmulator::wait_for_connection()
    {
        // NOLINTBEGIN (clang-analyzer-core.CallAndMessage)
        asio::co_spawn(*io_context_, listen_coro(), asio::detached);
        // NOLINTEND (clang-analyzer-core.CallAndMessage)
        // io_context_.join();
    }

    void SRSEmulator::wait_for_data_sender()
    {
        const auto _ = ExitLogger{ config_.ip };
        auto timer_waiter = [](std::shared_ptr<asio::system_timer> timer) -> asio::awaitable<void>
        { [[maybe_unused]] auto [err] = co_await timer->async_wait(asio::as_tuple(asio::use_awaitable)); };
        asio::co_spawn(*io_context_, timer_waiter(data_sender_wait_timer_), asio::use_future).get();
    }

    void SRSEmulator::do_if_acq_on(asio::any_io_executor& executor)
    {
        if (is_idle_.load())
        {
            is_idle_.store(true);

            // spdlog::trace("Server: start_send_data spawning ...");
            asio::co_spawn(executor, start_send_data(), asio::detached);
            // spdlog::trace("Server: start_send_data spawned!");
        }
    }

    void SRSEmulator::do_if_acq_off() { is_shutdown_.store(true); }

    auto SRSEmulator::send_response(connection::udp::endpoint endpoint, ReceiveType result_type)
        -> asio::awaitable<void>
    {
        const auto _ = ExitLogger{ config_.ip };
        auto msg = get_msg_from_receive_type(result_type);
        [[maybe_unused]] const auto size =
            co_await udp_socket_.async_send_to(asio::buffer(msg), endpoint, asio::use_awaitable);
        spdlog::trace("SRS server: sent the data {:02x} to the remote point {}", fmt::join(msg, " "), endpoint);
    }

    auto SRSEmulator::listen_coro() -> asio::awaitable<void>
    {
        const auto _ = ExitLogger{ config_.ip };
        auto msg_buffer = std::vector<char>{};
        msg_buffer.resize(common::SMALL_READ_MSG_BUFFER_SIZE);
        auto executor = co_await asio::this_coro::executor;
        for (;;)
        {
            spdlog::trace("Emulator: listening on the local port {}", udp_socket_.local_endpoint());
            auto remote_endpoint = asio::ip::udp::endpoint{};
            auto msg_size =
                co_await udp_socket_.async_receive_from(asio::buffer(msg_buffer), remote_endpoint, asio::use_awaitable);
            auto msg = std::string_view{ msg_buffer.data(), msg_size };
            auto result = check_receive_msg_type(msg);
            spdlog::trace("Emulator: Request type {:?} received with the msg: {:02x}",
                          magic_enum::enum_name(result),
                          fmt::join(msg, " "));
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
                    spdlog::trace("SRS server: existing coroutine");
                    co_return;
                    break;
            }
        }
    }

    auto SRSEmulator::start_send_data() -> asio::awaitable<void>
    {
        const auto _ = ExitLogger{ config_.ip };
        spdlog::info("Emulator: Starting to send data to port {} ...", config_.data_port);
        spdlog::info("Emulator: Delay time is set to be {} us", delay_time_.count());
        auto total_size = std::size_t{ 0 };
        auto executor = co_await asio::this_coro::executor;
        auto connection = connection::UDPWriterConnection{ *io_context_,
                                                           asio::ip::udp::endpoint{
                                                               asio::ip::make_address("127.0.0.1"),
                                                               static_cast<asio::ip::port_type>(config_.data_port) } };
        auto send_coro =
            common::create_coro_task([&connection]() { return connection.send_continuous_message(); }, executor);

        while (not is_shutdown_.load())
        {
            auto read_str = frame_reader_.read_one_frame();
            if (not read_str.has_value())
            {
                spdlog::critical("Emulator: error occurred from the reading the frame with msg: {}", read_str.error());
                break;
            }
            // Check if reached the end of the input file. If so, restarting from beginning.
            if (read_str.value().empty())
            {
                if (is_continue_.load())
                {
                    frame_reader_.reset();
                    auto read_str = frame_reader_.read_one_frame();
                }
                else
                {
                    // send emtpy string to finish
                    co_await send_coro.async_resume(std::optional<std::string_view>{}, asio::use_awaitable);
                    break;
                }
            }

            data_sending_control_.expires_after(delay_time_);
            co_await data_sending_control_.async_wait(asio::use_awaitable);

            auto send_size = co_await send_coro.async_resume(std::optional{ read_str.value() }, asio::use_awaitable);
            total_size += send_size.value_or(0);
            ++n_frames_sent_;
        }
        spdlog::info(
            "Emulator: reaching the end of send_data coroutine. Sent number of frames: {} and total data size: "
            "{} from {}",
            n_frames_sent_,
            total_size,
            udp_socket_.local_endpoint());
        data_sender_wait_timer_->cancel();
        data_sender_wait_timer_->expires_after(std::chrono::seconds{ 0 });
    }

} // namespace srs::test
