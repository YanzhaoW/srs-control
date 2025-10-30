#include "DataSocket.hpp"
#include "SpecialSocketBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/workflow/Handler.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>

namespace srs::connection
{
    DataSocket::DataSocket(int port_number, io_context_type& io_context, workflow::Handler* workflow)
        : SpecialSocket(port_number, io_context)
        , io_context_{ &io_context }
        , workflow_handler_{ workflow }
    {
        read_msg_buffer_.resize(common::LARGE_READ_MSG_BUFFER_SIZE);
    }

    void DataSocket::set_buffer_size(std::size_t buffer_size) { read_msg_buffer_.resize(buffer_size); }

    // WARN: is it really needed?
    void DataSocket::register_send_action_imp(asio::awaitable<void> action,
                                              std::shared_ptr<ConnectionType> /*connection*/)
    {
        asio::co_spawn(*io_context_, std::move(action), asio::detached);
    }

    void DataSocket::response_handler(const UDPEndpoint& /*endpoint*/, std::span<char> response)
    {
        workflow_handler_->read_data_once(response);
    }

    void DataSocket::cancel()
    {
        auto& timer = get_cancel_timer();
        timer.cancel();
    }
} // namespace srs::connection
