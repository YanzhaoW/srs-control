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
    /**
     * @brief Class to manage the production and consumption of #LargeBuffer asynchronously.
     *
     * A BufferQueue contains two subqueues, implemented by the third party library. The first subqueue, called
     * "valid_buffer_queue", stores the incoming buffer read the UDP socket, ready to be consumed by the analysis
     * taskflow. The second subqueue, called "trash_buffer_queue" stores the already analyzed buffer from the taskflow,
     * which can be given back to the UDP socket to read further UDP message.
     */
    class BufferQueue
    {
      public:
        /**
         * @brief Configuration for the buffer queue class.
         */
        struct Config
        {
            //!< The size of each buffer in the buffer queue.
            std::size_t buffer_size = common::LARGE_READ_MSG_BUFFER_SIZE;
            std::size_t reserve_size = 1;    //!< The number of buffers to be preallocated in the trash buffer queue
            std::size_t queue_capacity = 10; //!< The maximum number of buffers in both buffer queues.
        };

        /**
         * @brief Default constructor
         *
         * @param config Config object
         */
        BufferQueue(const Config& config);

        /**
         * @brief Abort the blocking operation from BufferQueue::pop()
         *
         * @see BufferQueue::pop
         */
        void abort();

        /**
         * @brief Try to frst push a buffer into the valid buffer queue and then replace it with a buffer from the trash
         * buffer queue.
         *
         * @param element The buffer object to be pushed and replaced.
         * @return True if the pushing succeeds.
         */
        auto try_emplace(LargeBuffer& element) -> bool;

        // block
        /**
         * @brief **Blocking** the current thread by first pushing the element to the trash buffer queue and then
         * replacing it with a buffer from the valid buffer queue.
         *
         * @param element The buffer object to be pushed and replaced.
         */
        void pop(LargeBuffer& element);

        /**
         * @brief Get the current size in the valid buffer queue.
         *
         * The size of the valid buffer queue means the number of buffers pushed to the queue.
         *
         * @return Size of the buffer queue.
         */
        // TODO: this is unsafe for tbb
        auto size() const -> auto { return valid_buffer_queue_.size(); }

        /**
         * @brief Get the current capacity of the buffer queue.
         *
         * The capacity represents the maximum number of buffers that can be pushed to the valid buffer queue.
         * @return Capacity
         */
        auto capacity() const -> auto { return config_.queue_capacity; }

        auto get_n_trash_recycle_failures_empty() const -> auto { return n_trash_recycle_failures_empty_; }
        auto get_n_trash_recycle_failures_full() const -> auto { return n_trash_recycle_failures_full_; }

        /**
         * @brief Getter of the config object.
         *
         * @return Const reference to the config object.
         * @see ref
         */
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
