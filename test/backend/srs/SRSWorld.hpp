#pragma once

#include "srs/SRSEmulator.hpp"
#include "srs/devices/Configuration.hpp"
#include "srs/workflow/Handler.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <thread>

namespace srs::test
{
    class World
    {
      public:
        explicit World(std::string_view input_filename);

        auto launch_server() -> std::optional<std::string>;

        auto set_continue_output(bool is_continue) { srs_server_->set_continue_output(is_continue); }

        auto get_server() -> auto* { return srs_server_.get(); }

        void init();

        auto get_app() -> App& { return app_; }
        [[nodiscard]] auto get_error_msg() const -> const auto& { return error_msg_; }
        [[nodiscard]] auto get_event_nums() const -> const auto& { return event_nums_; }

      private:
        App app_;
        Config& app_config_;
        std::unique_ptr<SRSEmulator> srs_server_;
        std::string error_msg_;
        uint64_t event_nums_ = 0;
        std::jthread srs_server_thread_;
    };

} // namespace srs::test
