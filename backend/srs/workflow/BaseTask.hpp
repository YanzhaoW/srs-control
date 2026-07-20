#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/sinks/DataWriterOptions.hpp"
#include "srs/utils/AppReport.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include <asio/experimental/coro.hpp>
#include <asio/use_awaitable.hpp>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <expected>
#include <fmt/ranges.h>
#include <gsl/gsl-lite.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace srs::process
{

    template <typename Input, typename Output>
    class BaseTask
    {
      public:
        using InputType = Input;
        using OutputType = Output;

        using enum DataConvertOptions;

        using RunResult = std::expected<OutputType, std::string_view>;

        explicit BaseTask(std::string_view name, DataConvertOptions prev_convert, std::size_t n_lines = 1)
            : previous_conversion_{ prev_convert }
            , name_{ name }
            , n_lines_{ n_lines }
        {
            spdlog::trace("Task: Creating {} task with {} pipelines", name_, n_lines);
            last_time_points_.resize(n_lines_);
            stats_.resize(n_lines_);
        }

        ~BaseTask()
        {
            if (task_report_ != nullptr)
            {
                task_report_->register_task_result(get_name(), stats_);
            }
        }

        [[nodiscard]] auto get_n_lines() const -> std::size_t { return n_lines_; }
        [[nodiscard]] auto get_required_conversion() const -> DataConvertOptions { return previous_conversion_; }
        [[nodiscard]] auto get_name() const -> std::string_view { return name_; };
        [[nodiscard]] auto get_name_str() const -> std::string { return std::string{ name_ }; };

        void set_report(AppReport* task_report)
        {
            assert(task_report != nullptr);
            task_report_ = task_report;
        }

        auto run_once(this auto&& self,
                      const OutputTo<InputType> auto& prev_data_converter,
                      std::size_t line_number = 0) -> RunResult
        {
            assert(line_number < self.get_n_lines());

            self.last_time_points_[line_number] = self.clock_.now();

            auto res = self.run(prev_data_converter, line_number);

            auto time_duration = self.clock_.now() - self.last_time_points_[line_number];
            self.stats_[line_number].total_time_ms += static_cast<double>(time_duration.count()) / 1e6;
            ++(self.stats_[line_number].total_sample_size);

            return res;
        }

      protected:
        auto get_report() -> AppReport* { return task_report_; }

      private:
        process::DataConvertOptions previous_conversion_;
        AppReport* task_report_ = nullptr;
        std::vector<AppReport::TaskStat> stats_;
        std::chrono::steady_clock clock_;
        std::vector<std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>> last_time_points_;
        std::string name_ = "empty";
        std::size_t n_lines_ = 1;
    };

    template <DataConvertOptions Conversion, typename Input, typename Output>
    class ConverterTask : public BaseTask<Input, Output>
    {
      public:
        constexpr static auto converter_type = Conversion;
        explicit ConverterTask(std::string_view name, DataConvertOptions prev_convert, std::size_t n_lines = 1)
            : BaseTask<Input, Output>{ name, prev_convert, n_lines }
        {
        }
    };

    template <DataWriterOption writer, typename Input, typename Output>
    class SinkTask : public BaseTask<Input, Output>
    {
      public:
        constexpr static auto writer_type = writer;
        explicit SinkTask(std::string_view name, DataConvertOptions prev_convert, std::size_t n_lines = 1)
            : BaseTask<Input, Output>{ name, prev_convert, n_lines }
        {
        }
    };
} // namespace srs::process
