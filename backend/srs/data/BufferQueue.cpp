#include "BufferQueue.hpp"
#include "srs/data/LargeBuffer.hpp"
#include <cstddef>
#include <fmt/format.h>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>

namespace srs
{
    BufferQueue::BufferQueue(const Config& config)
        : config_{ config }
    {
        valid_buffer_queue_.set_capacity(static_cast<long>(config.queue_capacity));
        trash_buffer_queue_.set_capacity(static_cast<long>(config.queue_capacity));
        print_memory_pre_allocation();
        for (const auto _ : std::views::iota(std::size_t{ 0 }, config_.reserve_size))
        {
            if (not trash_buffer_queue_.try_emplace(config_.buffer_size))
            {
                break;
            }
        }
        spdlog::debug("Memory preallocation finished.");
    }

    void BufferQueue::print_memory_pre_allocation()
    {
        const auto bytes_alloc = config_.buffer_size * config_.reserve_size;
        auto print_str = std::string{};
        if (bytes_alloc < 1000'000)
        {
            print_str = fmt::format("{} KB", bytes_alloc / 1000);
        }
        else if (bytes_alloc < 1000'000'000)
        {
            print_str = fmt::format("{} MB", bytes_alloc / 1000'000);
        }
        else
        {
            print_str = fmt::format("{} GB", bytes_alloc / 1000'000'000);
        }
        spdlog::info("Trying to pre-allocate {} memory for the buffer queue ...", print_str);
    }
    void BufferQueue::abort()
    {
        valid_buffer_queue_.abort();
        trash_buffer_queue_.abort();
    }

    // NOTE: try_emplace will use non-const copy constructor, which performs swap operator
    auto BufferQueue::try_emplace(LargeBuffer& element) -> bool
    {
        time_point_ = clock_.now();
        auto is_pushed = valid_buffer_queue_.try_emplace(element);
        if (is_pushed and not trash_buffer_queue_.try_pop(element))
        {
            ++n_trash_recycle_failures_empty_;
            element.resize(config_.buffer_size);
        }
        return is_pushed;
    }

    // block
    void BufferQueue::pop(LargeBuffer& element)
    {
        if (not trash_buffer_queue_.try_emplace(element))
        {
            ++n_trash_recycle_failures_full_;
        }
        valid_buffer_queue_.pop(element);
    }
} // namespace srs
