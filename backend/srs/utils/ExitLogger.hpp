#pragma once

#include "srs/utils/CommonDefinitions.hpp"
#include <source_location>
#include <spdlog/spdlog.h>
#include <string_view>

namespace srs
{

    class ExitLogger
    {
      public:
        explicit ExitLogger(std::string_view tag = "",
                            const std::source_location& location = std::source_location::current())
            : function_name_{ location.function_name() }
            , tag_{ tag }
        {
            if (tag_.empty())
            {
                tag_ = std::string_view{ "function" };
            }
            spdlog::trace("{} {} ({})", common::exit_logger_begin, function_name_, tag_);
        }

        ExitLogger(const ExitLogger&) = default;
        ExitLogger(ExitLogger&&) = delete;
        ExitLogger& operator=(const ExitLogger&) = default;
        ExitLogger& operator=(ExitLogger&&) = delete;
        ~ExitLogger() { spdlog::trace("{} {} ({})", common::exit_logger_end, function_name_, tag_); }

      private:
        std::string_view function_name_;
        std::string_view tag_;
    };

} // namespace srs
