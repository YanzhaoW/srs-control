#include <asio.hpp>
#include <srs/format.hpp>
#include <srs/srs.hpp>

constexpr auto PORT = "11111";
constexpr auto IP_ADDR = "localhost";
constexpr auto INPUT_BUFFER_SIZE = 10000;
using udp = asio::ip::udp;

namespace
{
    auto create_socket_from_ip_port(asio::io_context& context, const std::string& ip_addr, const std::string& port)
    {
        auto resolver = udp::resolver{ context };
        auto udp_endpoints = resolver.resolve(udp::v4(), ip_addr, port);
        auto endpoint = *udp_endpoints.begin();
        return udp::socket{ context, endpoint };
    }

} // namespace

class MsgReceiver
{
  public:
    explicit MsgReceiver(udp::socket socket)
        : socket_{ std::move(socket) }
    {
    }

    void listen()
    {
        socket_.async_receive(asio::buffer(msg_buffer_),
                              [this](auto error_code, auto byte_read)
                              {
                                  auto msg = std::string_view{ msg_buffer_.data(), byte_read };
                                  if (!error_code)
                                  {
                                      const auto& struct_data = msg_reader_.convert(msg);
                                      fmt::print("{}\n", struct_data);
                                  }
                                  listen();
                              });
    }

  private:
    udp::socket socket_;
    std::array<char, INPUT_BUFFER_SIZE> msg_buffer_{};
    srs::ProtoMsgReader msg_reader_;
};

auto main() -> int
{
    auto io_context = asio::io_context{};

    auto msg_receiver = MsgReceiver{ create_socket_from_ip_port(io_context, IP_ADDR, PORT) };
    msg_receiver.listen();
    io_context.run();

    return 0;
}
