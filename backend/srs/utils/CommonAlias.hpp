#pragma once

#include <array>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/cobalt.hpp>
#include <cstdint>
#include <srs/utils/CommonDefinitions.hpp>
#include <vector>

namespace srs
{
    namespace asio = boost::asio;
    namespace cobalt = boost::cobalt;

    using io_context_type = asio::thread_pool;
    using io_executor_type = asio::any_io_executor;

    using BufferElementType = char;
    using BinaryData = std::vector<BufferElementType>;

    template <int buffer_size = common::SMALL_READ_MSG_BUFFER_SIZE>
    using ReadBufferType = std::array<BufferElementType, buffer_size>;

    using CommunicateEntryType = uint32_t;
} // namespace srs
