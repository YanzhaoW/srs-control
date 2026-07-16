#include "DataSocket.hpp"
#include "SpecialSocketBase.hpp"
#include "srs/Application.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/workflow/AnalysisHandle.hpp"
#include <asio/awaitable.hpp>
#include <asio/detached.hpp>
#include <asio/impl/co_spawn.hpp>
#include <cstddef>
#include <memory>

namespace srs::connection
{
    DataSocket::DataSocket(int port_number,
                           io_context_type& io_context,
                           std::size_t buffer_size,
                           workflow::AnalysisHandle& workflow)
        : SpecialSocket(port_number, io_context)
        , buffer_size_{ buffer_size }
        , read_msg_buffer_{ buffer_size_ }
        , io_context_{ &io_context }
        , workflow_handler_{ &workflow }
        , token_{ workflow.get_queue_producer_token() }
    {
    }

    // WARN: is it really needed?
    void DataSocket::register_send_action_imp(asio::awaitable<void> /* action */,
                                              const std::shared_ptr<ConnectionType>& /*connection*/)
    {
        // asio::co_spawn(*io_context_, std::move(action), asio::detached);
    }

    void DataSocket::response_handler(const UDPEndpoint& /*endpoint*/, std::size_t read_size)
    {
        read_msg_buffer_.resize(read_size);
        workflow_handler_->read_data_once(read_msg_buffer_, token_);
        read_msg_buffer_.resize(buffer_size_);
    }

    void DataSocket::cancel()
    {
        const auto _ = ExitLogger{};
        auto& timer = get_cancel_timer();
        timer.cancel();
    }
} // namespace srs::connection
