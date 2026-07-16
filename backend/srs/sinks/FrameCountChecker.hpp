#pragma once

#include "DataWriterOptions.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/workflow/BaseTask.hpp"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace srs::sink
{
    class FrameCountChecker : public process::SinkTask<DataWriterOption::no_output, std::string_view, int64_t>
    {
      public:
        static constexpr auto IsStructType = false;

        FrameCountChecker(std::size_t n_lines);

        FrameCountChecker(const FrameCountChecker&) = delete;
        FrameCountChecker(FrameCountChecker&&) noexcept = delete;
        FrameCountChecker& operator=(const FrameCountChecker&) = delete;
        FrameCountChecker& operator=(FrameCountChecker&&) noexcept = delete;
        ~FrameCountChecker();

        auto run(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number = 0) -> RunResult
        {
            assert(line_number < get_n_lines());
            auto input_data = prev_data_converter(line_number);
            return analyze_frame_counter(input_data);
        }

        [[nodiscard]] auto get_frame_drop_count() const -> int64_t { return frame_drop_counter_.load(); }

        void close() {}

      private:
        std::atomic<int64_t> last_frame_counter_;
        std::atomic<int64_t> frame_drop_counter_;

        auto analyze_frame_counter(InputType binary_data) -> RunResult;
        auto extract_frame_counter(InputType binary_data) -> RunResult;
        void register_frame_counter(int64_t frame_counter);
    };
} // namespace srs::sink
