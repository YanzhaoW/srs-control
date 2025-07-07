#pragma once

#include <array>
#include <boost/asio/thread_pool.hpp>
#include <cstdint>
#include <srs/utils/CommonDefinitions.hpp>
#include <vector>

namespace srs
{
    namespace asio = boost::asio;

    using io_context_type = asio::thread_pool;

    using BufferElementType = char;
    using BinaryData = std::vector<BufferElementType>;

    template <int buffer_size = common::SMALL_READ_MSG_BUFFER_SIZE>
    using ReadBufferType = std::array<BufferElementType, buffer_size>;

    using CommunicateEntryType = uint32_t;
} // namespace srs
