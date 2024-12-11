#pragma once
#include <boost/asio/thread_pool.hpp>
#include <srs/utils/CommonDefitions.hpp>
#include <vector>

namespace srs
{
    /**  \defgroup CommonAliases Common aliases
     *   @{
     */

    namespace asio = boost::asio;

    using io_context_type = asio::thread_pool;

    using BufferElementType = char;
    using BinaryData = std::vector<BufferElementType>;

    template <int buffer_size = common::SMALL_READ_MSG_BUFFER_SIZE>
    using ReadBufferType = std::array<BufferElementType, buffer_size>;

    using CommunicateEntryType = uint32_t;

    /** @}*/
} // namespace srs
