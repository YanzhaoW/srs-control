#pragma once
#include <atomic>
#include <condition_variable>
#include <srs/utils/CommonDefitions.hpp>

namespace srs
{
    struct Status
    {
        std::atomic<bool> is_configured = false;
        std::atomic<bool> is_acq_on = false;
        std::atomic<bool> is_reading = false;
        std::atomic<bool> is_acq_off = false;
        std::atomic<bool> is_on_exit = false;
        // std::atomic<bool> is_already_exit = false;
        std::condition_variable status_change;

        auto wait_for_status(auto&& condition, std::chrono::seconds time_duration = common::DEFAULT_STATUS_WAITING_TIME_SECONDS)
            -> bool
        {
            using namespace std::string_literals;
            auto mutex = std::mutex{};
            while (not condition(*this))
            {
                auto lock = std::unique_lock<std::mutex>{ mutex };
                auto res = status_change.wait_for(lock, time_duration);
                if (res == std::cv_status::timeout)
                {
                    return false;
                }
            }
            return true;
        }
    };

}; // namespace srs
