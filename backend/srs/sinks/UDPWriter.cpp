#include "UDPWriter.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/UDPFormatters.hpp" // IWYU pragma: keep
#include "srs/workflow/BaseTask.hpp"
#include <asio/ip/udp.hpp>
#include <cstddef>
#include <fmt/format.h>
#include <memory>
#include <ranges>
#include <spdlog/spdlog.h>

namespace srs::sink
{
    UDP::UDP(io_context_type& io_context,
             asio::ip::udp::endpoint remote_endpoint,
             std::size_t n_lines,
             process::DataConvertOptions deser_mode)
        : SinkTask{ fmt::format("{}", remote_endpoint), deser_mode, n_lines }
    {
        connections_.reserve(n_lines);
        output_data_.resize(n_lines);
        for ([[maybe_unused]] const auto idx : std::views::iota(0, static_cast<int>(n_lines)))
        {
            connections_.emplace_back(std::make_unique<connection::UDPWriterConnection>(io_context, remote_endpoint));
        }
    }

    UDP::~UDP() noexcept
    {
        for (auto& connection : connections_)
        {
            connection->close();
        }
        spdlog::info("Writer: UDP socket writer to the remote socket {:?} is closed successfully.", get_name());
    }
} // namespace srs::sink
