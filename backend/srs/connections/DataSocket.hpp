#pragma once

#include "srs/Application.hpp"
#include "srs/connections/ConnectionBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/SpecialSocketBase.hpp"
#include "srs/utils/CommonAlias.hpp"
#include <boost/asio/awaitable.hpp>
#include <cstddef>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <span>
#include <vector>

namespace srs::connection
{
    class DataSocket : public SpecialSocket
    {
      public:
        using ConnectionType = CommandBase;
        void set_buffer_size(std::size_t buffer_size);
        void cancel();

      private:
        friend SpecialSocket;
        std::vector<char> read_msg_buffer_;
        gsl::not_null<io_context_type*> io_context_;
        gsl::not_null<workflow::Handler*> workflow_handler_;

        void register_send_action_imp(asio::awaitable<void> action, std::shared_ptr<ConnectionType> connection);
        auto get_response_msg_buffer() -> auto& { return read_msg_buffer_; }
        void response_handler(const UDPEndpoint& endpoint, std::span<char> response);
        static auto is_finished() -> bool { return false; };
        static void print_error() {};

        DataSocket(int port_number, io_context_type& io_context, workflow::Handler* workflow);
    };
} // namespace srs::connection
