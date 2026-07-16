#pragma once

#include "srs/Application.hpp"
#include "srs/connections/ConnectionBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/SpecialSocketBase.hpp"
#include "srs/data/BufferQueue.hpp"
#include "srs/data/LargeBuffer.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <asio/awaitable.hpp>
#include <cstddef>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <span>

namespace srs::connection
{
    /**
     * @brief Socket class for reading main data stream.
     */
    class DataSocket : public SpecialSocket
    {
      public:
        using ConnectionType = CommandBase;

        /**
         * @brief Cancel reading the data from the UDP socket.
         */
        void cancel();

      private:
        friend SpecialSocket;
        std::size_t buffer_size_ = common::LARGE_READ_MSG_BUFFER_SIZE;
        LargeBuffer read_msg_buffer_;
        gsl::not_null<io_context_type*> io_context_;
        gsl::not_null<workflow::AnalysisHandle*> workflow_handler_;
        BufferQueue::Token token_;

        void register_send_action_imp(asio::awaitable<void> action, const std::shared_ptr<ConnectionType>& connection);
        auto get_response_msg_buffer() -> std::span<char> { return read_msg_buffer_.get_all_data(); }
        void response_handler(const UDPEndpoint& endpoint, std::size_t read_size);
        static auto is_finished() -> bool { return false; };
        static void print_error() {};

        DataSocket(int port_number,
                   io_context_type& io_context,
                   std::size_t buffer_size,
                   workflow::AnalysisHandle& workflow);
    };
} // namespace srs::connection
