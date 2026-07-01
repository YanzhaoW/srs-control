#pragma once

#include "srs/data/LargeBuffer.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <chrono>
#include <cstddef>

#ifdef USE_ONEAPI_TBB
#include <oneapi/tbb/concurrent_queue.h>
#else
#include <tbb/concurrent_queue.h>
#endif

namespace srs
{
    class BufferQueue
    {
      public:
        struct Config
        {
            std::size_t buffer_size = common::LARGE_READ_MSG_BUFFER_SIZE;
            std::size_t reserve_size = 1;
            std::size_t queue_capacity = 10;
        };

        BufferQueue(const Config& config);
        void abort();
        auto try_emplace(LargeBuffer& element) -> bool;

        // block
        void pop(LargeBuffer& element);

        // TODO: this is unsafe for tbb
        auto size() const -> auto { return valid_buffer_queue_.size(); }

        auto capacity() const -> auto { return config_.queue_capacity; }

        auto get_n_trash_recycle_failures_empty() const -> auto { return n_trash_recycle_failures_empty_; }
        auto get_n_trash_recycle_failures_full() const -> auto { return n_trash_recycle_failures_full_; }
        [[nodiscard]] auto get_config() const -> const auto& { return config_; }

      private:
        Config config_;
        std::size_t n_trash_recycle_failures_empty_ = 0;
        std::size_t n_trash_recycle_failures_full_ = 0;
        tbb::concurrent_bounded_queue<LargeBuffer> valid_buffer_queue_;
        tbb::concurrent_bounded_queue<LargeBuffer> trash_buffer_queue_;

        std::chrono::steady_clock clock_;
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> time_point_;

        void print_memory_pre_allocation();
    };
} // namespace srs
