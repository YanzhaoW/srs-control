#pragma once

#include "srs/workflow/Handler.hpp"
#include "test/backend/utils/SRSEmulator.hpp"
#include <algorithm>
#include <asio/thread_pool.hpp>
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
        struct Config
        {
            bool is_continue = false;
            std::size_t delay_time = 1;
            std::size_t n_instances = 1;
            std::string_view input_filename;
        };

        explicit World(App& app, const Config& config);

        auto launch_server() -> std::optional<std::string>;

        void for_each_server(auto fnt) { std::ranges::for_each(srs_servers_, fnt); }

        void init();

        void join();

        auto get_n_frames_sent() const -> std::size_t
        {
            return std::ranges::fold_left(srs_servers_,
                                          std::size_t{},
                                          [](std::size_t num, const auto& server)
                                          { return num + server->get_n_frames_sent(); });
        }

        [[nodiscard]] auto get_error_msg() const -> const auto& { return error_msg_; }
        [[nodiscard]] auto get_event_nums() const -> const auto& { return event_nums_; }

      private:
        using IOContextType = asio::thread_pool;
        IOContextType io_context_{ 4 };
        gsl::not_null<App*> app_;
        Config config_;
        std::vector<std::unique_ptr<SRSEmulator>> srs_servers_;
        std::string error_msg_;
        uint64_t event_nums_ = 0;
        std::vector<std::jthread> srs_server_thread_;
    };

} // namespace srs::test
