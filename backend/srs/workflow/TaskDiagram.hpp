#pragma once

#include "srs/Application.hpp"
#include "srs/converters/ProtoSerializer.hpp"
#include "srs/converters/RawToDelimRawConveter.hpp"
#include "srs/converters/StructDeserializer.hpp"
#include "srs/converters/StructToProtoConverter.hpp"
#include "srs/data/BufferQueue.hpp"
#include "srs/data/LargeBuffer.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/sinks/Manager.hpp"
#include "srs/utils/AppReport.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <optional>
#include <string_view>
#include <taskflow/core/executor.hpp>
#include <taskflow/core/task.hpp>
#include <taskflow/core/taskflow.hpp>
#include <utility>
#include <vector>

namespace srs::workflow
{
    /**
     * @brief Class for analysis taskflow
     *
     */
    class TaskDiagram
    {
      public:
        /**
         * @brief Constructor with number of pipelines
         *
         * @param data_processor Pointer to srs::workflow::Handler
         * @param n_lines Number of pipelines
         */
        explicit TaskDiagram(AnalysisHandle& data_processor, std::size_t n_lines = 1);

        using InputType = bool;
        using OutputType = std::size_t;

        /**
         * @brief Run the taskflow and block the current thread until finished.
         */
        void construct_taskflow_and_run(BufferQueue& data_queue, const std::atomic<bool>& is_stopped);
        auto run_task(BufferQueue& data_queue, std::size_t line_number) -> bool;
        auto is_taskflow_abort_ready() const -> bool;
        auto is_done() const -> bool { return is_done_.load(); }
        [[nodiscard]] auto operator()(std::size_t line_number) const -> std::string_view
        {
            return raw_data_[line_number].data();
        }

        [[nodiscard]] auto get_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }
        [[nodiscard]] auto get_n_lines() const -> std::size_t { return n_lines_; }

        auto get_struct_data() -> const StructData*
        {
            if (struct_deserializer_converter_)
            {
                return struct_deserializer_converter_.value()(0);
            }
            return nullptr;
        }

        void register_report(AppReport& report) { report.register_task_result("Workflow", stats_); }

      private:
        std::atomic<bool> is_done_ = false;
        std::size_t n_lines_ = 1;
        tf::Executor tf_executor_;
        tf::Taskflow main_taskflow_;
        std::vector<tf::Taskflow> taskflow_lines_;
        std::vector<BufferQueue::Token> consumer_tokens_;
        std::vector<std::atomic<bool>> is_pipeline_stopped_;
        std::vector<LargeBuffer> raw_data_;

        std::optional<process::Raw2DelimRawConverter> raw_to_delim_raw_converter_;
        std::optional<process::StructDeserializer> struct_deserializer_converter_;
        std::optional<process::Struct2ProtoConverter> struct_to_proto_converter_;
        std::optional<process::ProtoSerializer> proto_serializer_converter_;
        std::optional<process::ProtoDelimSerializer> proto_delim_serializer_converter_;

        std::atomic<uint64_t> total_read_data_bytes_ = 0;
        std::vector<AppReport::TaskStat> stats_;
        using TimePoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;
        std::vector<TimePoint> last_times_;

        gsl::not_null<sink::Manager*> sinks_;
        AppReport* report_;

        void construct_taskflow_line(tf::Taskflow& taskflow, std::size_t line_number);

        void start_time_record(std::size_t line_num)
        {
            assert(line_num < last_times_.size());
            last_times_[line_num] = std::chrono::steady_clock::now();
        }

        void stop_time_record(std::size_t line_num)
        {
            assert(line_num < last_times_.size());
            ++(stats_[line_num].total_sample_size);
            stats_[line_num].total_time_ms +=
                static_cast<double>((std::chrono::steady_clock::now() - last_times_[line_num]).count()) / 1e6;
        }

        template <typename PrevConverter, typename ThisTask>
        auto emplace_to_taskflow(ThisTask& current_task,
                                 std::pair<const PrevConverter&, tf::Task>& prev_task,
                                 tf::Taskflow& taskflow,
                                 std::size_t line_number) -> std::optional<std::pair<const ThisTask&, tf::Task>>;

        template <typename PrevConverter, ConverterType ThisTask>
        auto create_task_imp(std::optional<ThisTask>& current_task,
                             std::optional<std::pair<const PrevConverter&, tf::Task>>& prev_task,
                             tf::Taskflow& taskflow,
                             std::size_t line_number) -> std::optional<std::pair<const ThisTask&, tf::Task>>;

        template <typename PrevConverter, SinkType ThisTask>
        auto create_task_imp(ThisTask& current_task,
                             std::optional<std::pair<const PrevConverter&, tf::Task>>& prev_task,
                             tf::Taskflow& taskflow,
                             std::size_t line_number) -> std::optional<std::pair<const ThisTask&, tf::Task>>;
    };
} // namespace srs::workflow
