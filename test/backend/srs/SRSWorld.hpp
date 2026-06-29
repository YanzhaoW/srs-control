#pragma once

#include "srs/SRSEmulator.hpp"
#include "srs/devices/Configuration.hpp"
#include "srs/workflow/Handler.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace srs::test
{
    class World
    {
      public:
        explicit World(App& app, std::string_view input_filename);

        auto launch_server() -> std::optional<std::string>;

        auto set_continue_output(bool is_continue)
        {
            std::ranges::for_each(srs_server_,
                                  [is_continue](auto& server) { server->set_continue_output(is_continue); });
        }

        auto set_delay_time_us(std::size_t time)
        {
            std::ranges::for_each(srs_server_, [time](auto& server) { server->set_delay_time(time); });
        }

        void for_each_server(auto fnt) { std::ranges::for_each(srs_server_, fnt); }

        void init();

        void join();

        [[nodiscard]] auto get_error_msg() const -> const auto& { return error_msg_; }
        [[nodiscard]] auto get_event_nums() const -> const auto& { return event_nums_; }

      private:
        gsl::not_null<App*> app_;
        std::vector<std::unique_ptr<SRSEmulator>> srs_server_;
        std::string error_msg_;
        uint64_t event_nums_ = 0;
        std::vector<std::jthread> srs_server_thread_;
    };

} // namespace srs::test
