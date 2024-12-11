#pragma once

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <fmt/ranges.h>
#include <gsl/gsl-lite.hpp>
#include <spdlog/spdlog.h>
#include <srs/Application.hpp>
#include <srs/converters/SerializableBuffer.hpp>
#include <srs/utils/CommonConcepts.hpp>
#include <srs/utils/CommonFunctions.hpp>
#include <srs/utils/ConnectionTypeDef.hpp>

namespace srs::connection
{
    // derive from enable_shared_from_this to make sure object still alive in the coroutine
    template <int buffer_size = common::SMALL_READ_MSG_BUFFER_SIZE>
    class Base : public std::enable_shared_from_this<Base<buffer_size>>
    {
      public:
        explicit Base(const Info& info, std::string name)
            : local_port_number_{ info.local_port_number }
            , name_{ std::move(name) }
            , app_{ info.control }
        {
            spdlog::debug("Creating connection {} with buffer size: {}", name_, buffer_size);
        }

        // possible overload from derived class
        void read_data_handle(std::span<BufferElementType> read_data) {}
        void close() { close_socket(); }
        void on_fail() { spdlog::debug("default on_fail is called!"); }
        auto get_executor() { return app_->get_io_context().get_executor(); }

        void listen(this auto&& self, bool is_non_stop = false);
        void communicate(this auto&& self, const std::vector<CommunicateEntryType>& data, uint16_t address);

        auto send_continuous_message() -> asio::experimental::coro<int(std::optional<std::string_view>)>;

        // Settters:
        void set_socket(std::unique_ptr<asio::ip::udp::socket> socket) { socket_ = std::move(socket); }
        void set_remote_endpoint(asio::ip::udp::endpoint endpoint) { remote_endpoint_ = std::move(endpoint); }
        void set_timeout_seconds(int val) { timeout_seconds_ = val; }

        void set_send_message(const RangedData auto& msg)
        {
            continuous_send_msg_ = std::span{ msg.begin(), msg.end() };
        }

        // Getters:
        [[nodiscard]] auto get_read_msg_buffer() const -> const ReadBufferType<buffer_size>&
        {
            return read_msg_buffer_;
        }
        [[nodiscard]] auto get_name() const -> const std::string& { return name_; }
        [[nodiscard]] auto get_app() -> App& { return *app_; }
        auto get_socket() -> const udp::socket& { return *socket_; }
        auto get_remote_endpoint() -> const udp::endpoint& { return remote_endpoint_; }
        [[nodiscard]] auto get_local_port_number() const -> int { return local_port_number_; }
        [[nodiscard]] auto is_continuous() const -> bool { return is_continuous_; }

      protected:
        auto new_shared_socket(int port_number) -> std::unique_ptr<udp::socket>;
        void close_socket();
        void set_continuous(bool is_continuous = true) { is_continuous_ = is_continuous; }

      private:
        bool is_continuous_ = false;
        int local_port_number_ = 0;
        std::atomic<bool> is_socket_closed_{ false };
        uint32_t counter_ = common::INIT_COUNT_VALUE;
        std::string name_ = "ConnectionBase";
        gsl::not_null<App*> app_;
        std::unique_ptr<udp::socket> socket_;
        udp::endpoint remote_endpoint_;
        process::SerializableMsgBuffer write_msg_buffer_;
        std::span<const char> continuous_send_msg_;
        std::unique_ptr<asio::signal_set> signal_set_;
        ReadBufferType<buffer_size> read_msg_buffer_{};
        int timeout_seconds_ = common::DEFAULT_TIMEOUT_SECONDS;

        void encode_write_msg(const std::vector<CommunicateEntryType>& data, uint16_t address);
        static auto signal_handling(SharedConnectionPtr auto connection) -> asio::awaitable<void>;
        static auto timer_countdown(auto* connection) -> asio::awaitable<void>;
        static auto listen_message(SharedConnectionPtr auto connection, bool is_non_stop = false)
            -> asio::awaitable<void>;
        static auto send_message(std::shared_ptr<Base> connection) -> asio::awaitable<void>;
        void reset_read_msg_buffer() { std::fill(read_msg_buffer_.begin(), read_msg_buffer_.end(), 0); }
    };

    // Member function definitions:

    template <int buffer_size>
    auto Base<buffer_size>::send_message(std::shared_ptr<Base<buffer_size>> connection) -> asio::awaitable<void>
    {
        spdlog::trace("Connection {}: Sending data ...", connection->get_name());
        auto data_size = co_await connection->socket_->async_send_to(
            asio::buffer(connection->write_msg_buffer_.data()), connection->remote_endpoint_, asio::use_awaitable);
        spdlog::debug("Connection {}: Message is sent.", connection->get_name());
        spdlog::trace("Connection {}: {} bytes data sent with {:02x}",
                      connection->get_name(),
                      data_size,
                      fmt::join(connection->write_msg_buffer_.data(), " "));
    }

    template <int buffer_size>
    auto Base<buffer_size>::send_continuous_message() -> asio::experimental::coro<int(std::optional<std::string_view>)>
    {
        auto send_msg = std::string_view{};
        auto data_size = 0;
        while (true)
        {
            data_size = (not send_msg.empty()) ? socket_->send_to(asio::buffer(send_msg), remote_endpoint_) : 0;
            auto msg = co_yield data_size;

            if (not msg.has_value())
            {
                close();
                co_return;
            }
            else
            {
                send_msg = msg.value();
            }
        }
    }

    template <int buffer_size>
    auto Base<buffer_size>::listen_message(SharedConnectionPtr auto connection, bool is_non_stop)
        -> asio::awaitable<void>
    {
        using asio::experimental::awaitable_operators::operator||;

        spdlog::debug("Connection {}: starting to listen ...", connection->get_name());

        auto io_context = co_await asio::this_coro::executor;
        auto timer = asio::steady_timer{ io_context };
        if (is_non_stop)
        {
            timer.expires_at(decltype(timer)::time_point::max());
        }
        else
        {
            timer.expires_after(std::chrono::seconds{ connection->timeout_seconds_ });
        }

        while (true)
        {
            if (not connection->socket_->is_open() or connection->is_socket_closed_.load())
            {
                co_return;
            }

            auto receive_data_size = co_await (
                connection->socket_->async_receive(asio::buffer(connection->read_msg_buffer_), asio::use_awaitable) ||
                timer.async_wait(asio::use_awaitable));
            if (not is_non_stop and std::holds_alternative<std::monostate>(receive_data_size))
            {
                if (not connection->is_continuous())
                {
                    spdlog::error("Connection {}: Message listening TIMEOUT after {} seconds.",
                                  connection->get_name(),
                                  connection->timeout_seconds_);
                    connection->on_fail();
                }
                else
                {
                    spdlog::info("Connection {}: Message listening TIMEOUT after {} seconds.",
                                 connection->get_name(),
                                 connection->timeout_seconds_);
                }
                break;
            }
            auto read_msg = std::span{ connection->read_msg_buffer_.data(), std::get<std::size_t>(receive_data_size) };
            connection->read_data_handle(read_msg);
            // spdlog::info("Connection {}: received {} bytes data", connection->get_name(), read_msg.size());

            connection->reset_read_msg_buffer();
            if (not connection->is_continuous() or connection->get_app().get_status().is_on_exit.load())
            {
                break;
            }
        }
        connection->close();
    }

    template <int buffer_size>
    auto Base<buffer_size>::signal_handling(SharedConnectionPtr auto connection) -> asio::awaitable<void>
    {
        connection->signal_set_ = std::make_unique<asio::signal_set>(co_await asio::this_coro::executor, SIGINT);
        spdlog::trace("Connection {}: waiting for signals", connection->get_name());
        auto [error, sig_num] = co_await connection->signal_set_->async_wait(asio::as_tuple(asio::use_awaitable));
        if (error == asio::error::operation_aborted)
        {
            spdlog::trace("Connection {}: Signal ended with {}", connection->get_name(), error.message());
        }
        else
        {
            fmt::print("\n");
            spdlog::trace(
                "Connection {}: Signal ID {} is called with {:?}!", connection->get_name(), sig_num, error.message());
            connection->close();
        }
    }

    template <int buffer_size>
    auto Base<buffer_size>::new_shared_socket(int port_number) -> std::unique_ptr<udp::socket>
    {
        auto socket = std::make_unique<udp::socket>(
            app_->get_io_context(), udp::endpoint{ udp::v4(), static_cast<asio::ip::port_type>(port_number) });
        spdlog::debug("Connection {}: Openning the socket from ip: {} with port: {}",
                      name_,
                      socket->local_endpoint().address().to_string(),
                      socket->local_endpoint().port());
        local_port_number_ = socket->local_endpoint().port();
        return socket;
    }

    template <int size>
    void Base<size>::encode_write_msg(const std::vector<CommunicateEntryType>& data, uint16_t address)
    {
        write_msg_buffer_.serialize(counter_,
                                    common::ZERO_UINT16_PADDING,
                                    address,
                                    common::WRITE_COMMAND_BITS,
                                    common::DEFAULT_TYPE_BITS,
                                    common::COMMAND_LENGTH_BITS);
        write_msg_buffer_.serialize(data);
    }

    template <int buffer_size>
    void Base<buffer_size>::listen(this auto&& self, bool is_non_stop)
    {
        using asio::experimental::awaitable_operators::operator||;
        if (self.socket_ == nullptr)
        {
            self.socket_ = self.new_shared_socket(self.local_port_number_);
        }

        co_spawn(self.app_->get_io_context(),
                 signal_handling(common::get_shared_from_this(self)) ||
                     listen_message(common::get_shared_from_this(self), is_non_stop),
                 asio::detached);
        spdlog::trace("Connection {}: spawned listen coroutine", self.name_);
    }

    template <int buffer_size>
    void Base<buffer_size>::communicate(this auto&& self,
                                        const std::vector<CommunicateEntryType>& data,
                                        uint16_t address)
    {
        self.listen();
        self.encode_write_msg(data, address);
        co_spawn(self.app_->get_io_context(), send_message(self.shared_from_this()), asio::detached);
        spdlog::trace("Connection {}: spawned write coroutine", self.name_);
    }

    template <int buffer_size>
    void Base<buffer_size>::close_socket()
    {
        if (not is_socket_closed_.load())
        {
            is_socket_closed_.store(true);
            spdlog::trace("Connection {}: Closing the socket ...", name_);
            // socket_->cancel();
            socket_->close();
            if (signal_set_ != nullptr)
            {
                spdlog::trace("Connection {}: cannelling signal ...", name_);
                signal_set_->cancel();
                spdlog::trace("Connection {}: signal is cancelled.", name_);
            }
            spdlog::trace("Connection {}: Socket is closed and cancelled.", name_);
        }
    }
} // namespace srs::connection
