#pragma once

#include <array>
#include <asio/any_io_executor.hpp>
#include <asio/thread_pool.hpp>
#include <cstdint>
#include <srs/utils/CommonDefinitions.hpp>
#include <vector>

namespace srs
{
    using io_context_type = asio::thread_pool;
    using io_executor_type = asio::any_io_executor;

    using BufferElementType = char;
    using BinaryData = std::vector<BufferElementType>;

    template <int buffer_size = common::SMALL_READ_MSG_BUFFER_SIZE>
    using ReadBufferType = std::array<BufferElementType, buffer_size>;

    using CommunicateEntryType = uint32_t;
} // namespace srs
