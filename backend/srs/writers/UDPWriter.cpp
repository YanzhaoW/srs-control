#include "UDPWriter.hpp"
#include "srs/connections/ConnectionBase.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <boost/asio/ip/udp.hpp>
#include <cstddef>
#include <fmt/format.h>
#include <memory>
#include <ranges>
#include <utility>

namespace srs::writer
{
    UDP::UDP(io_context_type& io_context,
             asio::ip::udp::endpoint remote_endpoint,
             std::size_t n_lines,
             process::DataConvertOptions deser_mode)
        : WriterTask{ fmt::format("{}", remote_endpoint), deser_mode, n_lines }
    {
        connections_.reserve(n_lines);
        for ([[maybe_unused]] const auto idx : std::views::iota(0, static_cast<int>(n_lines)))
        {
            auto& connection = connections_.emplace_back(std::make_unique<connection::UDPWriterConnection>(
                connection::Base::Config{ .name = "UDP writer", .buffer_size = common::LARGE_READ_MSG_BUFFER_SIZE },
                io_context));
            connection->set_socket(
                std::make_unique<asio::ip::udp::socket>(io_context, asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 }));
            connection->set_remote_endpoint(std::move(remote_endpoint));
        }
    }

    UDP::~UDP()
    {
        for (auto& connection : connections_)
        {
            connection->close();
        }
    }
} // namespace srs::writer
