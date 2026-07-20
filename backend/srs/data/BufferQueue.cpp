#include "BufferQueue.hpp"
#include "srs/data/LargeBuffer.hpp"
#include "srs/utils/AppReport.hpp"
#include <atomic>
#include <cstddef>
#include <fmt/format.h>
#include <iterator>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

namespace srs
{
    BufferQueue::BufferQueue(const Config& config)
        : config_{ config }
        , valid_buffer_queue_{ config.queue_capacity }
        , trash_buffer_queue_{ config.queue_capacity }
    {

        print_memory_pre_allocation();
        for (const auto _ : std::views::iota(std::size_t{ 0 }, config_.reserve_size))
        {
            if (not trash_buffer_queue_.try_enqueue(internal_trash_buffer_token_, LargeBuffer{ config_.buffer_size }))
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
    void BufferQueue::enqueue_empty(std::size_t bulk_size)
    {
        auto buffers = std::vector<LargeBuffer>{};
        for (auto _ : std::views::iota(std::size_t{ 0 }, bulk_size))
        {
            buffers.emplace_back();
        }
        valid_buffer_queue_.enqueue_bulk(
            internal_valid_buffer_token_, std::make_move_iterator(buffers.begin()), bulk_size);
    }

    // NOTE: try_enqueue will use move constructor, which performs swap operator
    auto BufferQueue::enqueue(LargeBuffer& element, Token& token) -> bool
    {
        if (not valid_buffer_queue_.try_enqueue(token.first, std::move(element)))
        {
            ++n_valid_buffer_full_failures_;
            return false;
        }

        if (not trash_buffer_queue_.try_dequeue(token.second, element))
        {
            ++n_trash_recycle_empty_failures_;
            element.resize(config_.buffer_size);
        }
        return true;
    }

    // block
    void BufferQueue::dequeue(LargeBuffer& element, Token& token)
    {
        if (not trash_buffer_queue_.try_enqueue(token.first, std::move(element)))
        {
            ++n_trash_recycle_full_failures_;
        }
        valid_buffer_queue_.wait_dequeue(token.second, element);
    }

    void BufferQueue::register_report(AppReport& report)
    {
        report.register_queue_result({ .empty_trash = n_trash_recycle_empty_failures_.load(std::memory_order_relaxed),
                                       .full_trash = n_trash_recycle_full_failures_.load(std::memory_order_relaxed),
                                       .full_valid = n_valid_buffer_full_failures_.load(std::memory_order_relaxed) });
    }
} // namespace srs
