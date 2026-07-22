#include "srs/workflow/TaskDiagram.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/data/BufferQueue.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/workflow/AnalysisHandle.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <taskflow/algorithm/pipeline.hpp>
#include <taskflow/core/task.hpp>
#include <taskflow/core/taskflow.hpp>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace srs::workflow
{
    TaskDiagram::TaskDiagram(AnalysisHandle& handle, std::size_t n_lines)
        : n_lines_{ n_lines }
        , is_pipeline_stopped_{ std::vector<std::atomic<bool>>(n_lines_) }
        , sinks_{ &handle.get_sink_manager_ref() }
        , report_{ &handle.get_app_ref().get_report() }
    {
        assert(report_ != nullptr);
        const auto buffer_size = handle.get_buffer_queue().get_config().buffer_size;
        consumer_tokens_.reserve(n_lines_);
        for (auto _ : std::views::iota(std::size_t{ 0 }, n_lines_))
        {
            consumer_tokens_.push_back(handle.get_queue_consumer_token());
            raw_data_.emplace_back(buffer_size);
        }

        for (auto& is_pipe_stopped : is_pipeline_stopped_)
        {
            is_pipe_stopped.store(true);
        }

        stats_.resize(n_lines);
        last_times_.resize(n_lines);
    }

    // blocking here with pop
    auto TaskDiagram::run_task(BufferQueue& buffer_queue, std::size_t line_number) -> bool
    {
        // raw_data_[line_number].clear();
        buffer_queue.dequeue(raw_data_[line_number], consumer_tokens_[line_number]);
        if (not raw_data_[line_number].is_empty())
        {
            total_read_data_bytes_ += raw_data_[line_number].get_size();
            return true;
        }
        return false;
    }

    void TaskDiagram::construct_taskflow_and_run(BufferQueue& buffer_queue, const std::atomic<bool>&)
    {
        const auto _ = ExitLogger{};
        taskflow_lines_.resize(n_lines_);
        for (const auto [line_number, taskflow] : std::views::zip(std::views::iota(0), taskflow_lines_))
        {
            construct_taskflow_line(taskflow, static_cast<std::size_t>(line_number));
        }

        auto starting_task =
            main_taskflow_
                .emplace(
                    [this]()
                    {
                        const auto& conversion_req_map = sinks_->generate_conversion_req_map();
                        spdlog::debug("Starting taskflow with enabled writers");
                        spdlog::debug("Conversion requirements map: \n"
                                      "\t|{:^30}|{:^30}|\n"
                                      "{}",
                                      "Conversions",
                                      "Required",
                                      fmt::join(conversion_req_map | std::views::transform(
                                                                         [](const auto conv_req) -> std::string
                                                                         {
                                                                             return fmt::format(
                                                                                 "\t|{:<30}|{:^29}|",
                                                                                 magic_enum::enum_name(conv_req.first),
                                                                                 conv_req.second ? "✅" : "❌");
                                                                         }),
                                                "\n"));
                    })
                .name("Starting");

        auto main_pipeline =
            tf::Pipeline{ n_lines_,
                          tf::Pipe{ tf::PipeType::SERIAL, []([[maybe_unused]] tf::Pipeflow& pipeflow) {} },
                          tf::Pipe{ tf::PipeType::PARALLEL,
                                    [this, &buffer_queue]([[maybe_unused]] tf::Pipeflow& pipeflow)
                                    {
                                        if (not run_task(buffer_queue, pipeflow.line()))
                                        {
                                            pipeflow.stop();
                                        }
                                        is_pipeline_stopped_[pipeflow.line()].store(false);
                                        start_time_record(pipeflow.line());
                                        tf_executor_.corun(taskflow_lines_[pipeflow.line()]);
                                        stop_time_record(pipeflow.line());
                                        is_pipeline_stopped_[pipeflow.line()].store(true);
                                    } },
                          tf::Pipe{ tf::PipeType::SERIAL, []([[maybe_unused]] tf::Pipeflow& pipeflow) {} } };

        auto pipeline_task = main_taskflow_.composed_of(main_pipeline).name("Main pipeline");
        starting_task.precede(pipeline_task);
        tf_executor_.run(main_taskflow_).wait();
        is_done_.store(true);
    }

    auto TaskDiagram::is_taskflow_abort_ready() const -> bool
    {
        static constexpr auto SLEEP_TIME = std::chrono::milliseconds(10);
        std::this_thread::sleep_for(SLEEP_TIME);
        auto res = std::ranges::all_of(is_pipeline_stopped_,
                                       [](const std::atomic<bool>& is_pipe_stopped) -> bool
                                       { return is_pipe_stopped.load(); });
        return res;
    }

    template <typename PrevConverter, typename ThisTask>
    auto TaskDiagram::emplace_to_taskflow(ThisTask& current_task,
                                          std::pair<const PrevConverter&, tf::Task>& prev_task,
                                          tf::Taskflow& taskflow,
                                          std::size_t line_number)
        -> std::optional<std::pair<const ThisTask&, tf::Task>>
    {
        current_task.set_report(report_);
        // NOTE: This is where the previous converter and current task is connected.
        auto task = taskflow
                        .emplace([line_number, &current_task, &prev_converter = prev_task.first]()
                                 { [[maybe_unused]] auto res = current_task.run_once(prev_converter, line_number); })
                        .name(current_task.get_name_str());
        if (not prev_task.second.empty())
        {
            prev_task.second.precede(task);
        }
        return std::pair<const ThisTask&, tf::Task>{ current_task, task };
    }

    template <typename PrevConverter, ConverterType ThisTask>
    auto TaskDiagram::create_task_imp(std::optional<ThisTask>& current_task,
                                      std::optional<std::pair<const PrevConverter&, tf::Task>>& prev_task,
                                      tf::Taskflow& taskflow,
                                      std::size_t line_number) -> std::optional<std::pair<const ThisTask&, tf::Task>>
    {
        if (not prev_task.has_value())
        {
            return {};
        }
        auto is_required = sinks_->is_convert_required(ThisTask::converter_type);
        if (not is_required)
        {
            return {};
        }
        if (not current_task)
        {
            current_task.emplace(n_lines_);
        }
        return emplace_to_taskflow(current_task.value(), prev_task.value(), taskflow, line_number);
    }

    template <typename PrevConverter, SinkType ThisTask>
    auto TaskDiagram::create_task_imp(ThisTask& current_task,
                                      std::optional<std::pair<const PrevConverter&, tf::Task>>& prev_task,
                                      tf::Taskflow& taskflow,
                                      std::size_t line_number) -> std::optional<std::pair<const ThisTask&, tf::Task>>
    {
        if (not prev_task.has_value())
        {
            return {};
        }
        return emplace_to_taskflow(current_task, prev_task.value(), taskflow, line_number);
    }

    void TaskDiagram::construct_taskflow_line(tf::Taskflow& taskflow, std::size_t line_number)
    {
        auto create_task = [line_number, &taskflow, this]<typename PrevConverter, typename ThisTask>(
                               ThisTask& current_task,
                               std::optional<std::pair<const PrevConverter&, tf::Task>>& prev_task)
            -> std::optional<std::pair<const remove_optional_t<ThisTask>&, tf::Task>>
        { return create_task_imp(current_task, prev_task, taskflow, line_number); };

        auto empty_task = std::optional{ std::pair<const TaskDiagram&, tf::Task>{ *this, tf::Task{} } };
        auto raw_delimiter_task = create_task(raw_to_delim_raw_converter_, empty_task);
        auto struct_deser_task = create_task(struct_deserializer_converter_, empty_task);
        auto struct_to_proto_task = create_task(struct_to_proto_converter_, struct_deser_task);
        auto proto_serial_task = create_task(proto_serializer_converter_, struct_to_proto_task);
        auto proto_delim_serial_task = create_task(proto_delim_serializer_converter_, struct_to_proto_task);
        // TODO: root_deser

        sinks_->do_for_each_sink(
            [&create_task,
             &empty_task,
             &raw_delimiter_task,
             &struct_deser_task,
             &proto_serial_task,
             &proto_delim_serial_task](std::string_view filename, auto& sink) -> void
            {
                if constexpr (std::remove_cvref_t<decltype(sink)>::IsStructType)
                {
                    create_task(sink, struct_deser_task);
                }
                else
                {
                    using enum process::DataConvertOptions;
                    const auto convert_mode = sink.get_required_conversion();
                    switch (convert_mode)
                    {
                        case raw:
                            create_task(sink, empty_task);
                            break;
                        case raw_frame:
                            create_task(sink, raw_delimiter_task);
                            break;
                        case proto:
                            create_task(sink, proto_serial_task);
                            break;
                        case proto_frame:
                            create_task(sink, proto_delim_serial_task);
                            break;
                        default:
                            spdlog::warn("unrecognized conversion {} from the file {}", convert_mode, filename);
                            create_task(sink, empty_task);
                            break;
                    }
                }
            });
    }
} // namespace srs::workflow
