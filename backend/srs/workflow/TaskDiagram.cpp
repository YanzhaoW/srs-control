#include "srs/workflow/TaskDiagram.hpp"
#include "srs/Application.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include <algorithm>
#include <atomic>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/thread/future.hpp>
#include <cstddef>
#include <expected>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <oneapi/tbb/concurrent_queue.h>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <taskflow/algorithm/pipeline.hpp>
#include <taskflow/core/task.hpp>
#include <taskflow/core/taskflow.hpp>
#include <type_traits>
#include <utility>
#include <vector>

namespace srs::workflow
{
    TaskDiagram::TaskDiagram(Handler* data_processor, asio::thread_pool& thread_pool, std::size_t n_lines)
        : n_lines_{ n_lines }
        , is_pipeline_stopped_{ std::vector<std::atomic<bool>>(n_lines_) }
        , raw_to_delim_raw_converter_{ n_lines }
        , struct_deserializer_converter_{ n_lines }
        , struct_to_proto_converter_{ n_lines }
        , proto_serializer_converter_{ n_lines }
        , proto_delim_serializer_converter_{ n_lines }
        , io_context_{ thread_pool.get_executor() }
        , writers_{ data_processor }
    {
        for (auto& is_pipe_stopped : is_pipeline_stopped_)
        {
            is_pipe_stopped.store(true);
        }
        raw_data_.resize(n_lines);
    }

    TaskDiagram::~TaskDiagram()
    {
        spdlog::debug("Task Workflow: stopping analysis ...");
        reset();
        // auto res = run_processes(true);
        auto res = run_workflow(true);
        spdlog::debug("Task Workflow: analysis is stopped ...");
        if (not res.has_value())
        {
            spdlog::error("{}", res.error());
        }
    }

    void TaskDiagram::run_task(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
                               std::size_t line_number)
    {
        raw_data_[line_number].clear();
        data_queue.pop(raw_data_[line_number]);
    }

    void TaskDiagram::construct_taskflow_and_run(
        tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
        const std::atomic<bool>& is_stopped)
    {
        taskflow_lines_.resize(n_lines_);
        for (const auto [line_number, taskflow] : std::views::enumerate(taskflow_lines_))
        {
            construct_taskflow_line(taskflow, static_cast<std::size_t>(line_number));
        }

        auto starting_task =
            main_taskflow_
                .emplace(
                    [this]()
                    {
                        const auto& conversion_req_map = writers_.generate_conversion_req_map();
                        spdlog::debug("Starting taskflow with enabled writers");
                        spdlog::debug("Conversion requirements map: \n\t{}",
                                      fmt::join(conversion_req_map | std::views::transform(
                                                                         [](const auto conv_req) -> std::string
                                                                         {
                                                                             return fmt::format(
                                                                                 "conversion \t {:<20} \t required: {}",
                                                                                 magic_enum::enum_name(conv_req.first),
                                                                                 conv_req.second);
                                                                         }),
                                                "\n\t"));
                    })
                .name("Starting");

        auto main_pipeline =
            tf::Pipeline{ n_lines_,
                          tf::Pipe{ tf::PipeType::SERIAL, []([[maybe_unused]] tf::Pipeflow& pipeflow) {} },
                          tf::Pipe{ tf::PipeType::PARALLEL,
                                    [this, &data_queue, &is_stopped]([[maybe_unused]] tf::Pipeflow& pipeflow)
                                    {
                                        if (is_stopped.load())
                                        {
                                            pipeflow.stop();
                                        }
                                        else
                                        {
                                            run_task(data_queue, pipeflow.line());
                                            is_pipeline_stopped_[pipeflow.line()].store(false);
                                            tf_executor_.corun(taskflow_lines_[pipeflow.line()]);
                                            is_pipeline_stopped_[pipeflow.line()].store(true);
                                        }
                                    } },
                          tf::Pipe{ tf::PipeType::SERIAL, []([[maybe_unused]] tf::Pipeflow& pipeflow) {} } };

        auto pipeline_task = main_taskflow_.composed_of(main_pipeline).name("Main pipeline");
        starting_task.precede(pipeline_task);
        tf_executor_.run(main_taskflow_).wait();
    }

    auto TaskDiagram::is_taskflow_abort_ready() const -> bool
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto res = std::ranges::all_of(is_pipeline_stopped_,
                                       [](const std::atomic<bool>& is_pipe_stopped) -> bool
                                       { return is_pipe_stopped.load(); });
        return res;
    }

    void TaskDiagram::construct_taskflow_line(tf::Taskflow& taskflow, std::size_t line_number)
    {
        // TODO: Add converter concept here
        auto create_task =
            [&writers = writers_, line_number, &taskflow]<typename PrevConverter, typename ThisConverter>(
                ThisConverter& converter, std::optional<std::pair<const PrevConverter&, tf::Task>>& prev_task)
            -> std::optional<std::pair<const ThisConverter&, tf::Task>>
        {
            if (not prev_task.has_value())
            {
                return {};
            }
            auto is_required = writers.is_convert_required(converter.get_required_conversion());
            if (not is_required)
            {
                return {};
            }
            auto task = taskflow
                            .emplace([line_number, &converter, &prev_converter = prev_task.value().first]()
                                     { converter.run_task(prev_converter, line_number); })
                            .name(converter.get_name_str());
            if (not prev_task.value().second.empty())
            {
                prev_task.value().second.precede(task);
            }
            return std::pair<const ThisConverter&, tf::Task>{ converter, task };
        };

        auto empty_task = std::optional{ std::pair<const TaskDiagram&, tf::Task>{ *this, tf::Task{} } };
        auto raw_delimiter_task = create_task(raw_to_delim_raw_converter_, empty_task);
        auto struct_deser_task = create_task(struct_deserializer_converter_, empty_task);
        auto struct_to_proto_task = create_task(struct_to_proto_converter_, struct_deser_task);
        auto proto_deser_task = create_task(proto_serializer_converter_, struct_to_proto_task);
        auto proto_delim_deser_task = create_task(proto_delim_serializer_converter_, struct_to_proto_task);
        // TODO: root_deser

        writers_.do_for_each_writer(
            [&create_task,
             &empty_task,
             &raw_delimiter_task,
             &struct_deser_task,
             &proto_deser_task,
             &proto_delim_deser_task](std::string_view filename, auto& file_writer) -> void
            {
                if constexpr (std::remove_cvref_t<decltype(file_writer)>::IsStructType)
                {
                    create_task(file_writer, struct_deser_task);
                }
                else
                {
                    using enum process::DataConvertOptions;
                    const auto convert_mode = file_writer.get_required_conversion();
                    switch (convert_mode)
                    {
                        case raw:
                            create_task(file_writer, empty_task);
                            break;
                        case raw_frame:
                            create_task(file_writer, raw_delimiter_task);
                            break;
                        case proto:
                            create_task(file_writer, proto_deser_task);
                            break;
                        case proto_frame:
                            create_task(file_writer, proto_delim_deser_task);
                            break;
                        default:
                            spdlog::warn("unrecognized conversion  {}from the file {}", convert_mode, filename);
                            create_task(file_writer, empty_task);
                            break;
                    }
                }
            });
    }

    auto TaskDiagram::analysis_one(
        [[maybe_unused]] tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue) -> bool
    {

        return true;
    }

    // auto TaskDiagram::analysis_one(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
    //                                bool is_blocking) -> bool
    // {
    //     auto pop_res = true;
    //     reset();
    //     if (is_blocking)
    //     {
    //         data_queue.pop(binary_data_);
    //     }
    //     else
    //     {
    //         pop_res = data_queue.try_pop(binary_data_);
    //     }
    //     if (pop_res)
    //     {
    //         total_read_data_bytes_ += binary_data_.data().size();
    //         // auto res = run_processes(false);
    //         auto res = run_workflow(false);
    //         if (not res.has_value())
    //         {
    //             throw std::runtime_error(fmt::format("{}", res.error()));
    //         }
    //     }
    //     return pop_res;
    // }

    auto TaskDiagram::run_processes([[maybe_unused]] bool is_stopped) -> std::expected<void, std::string_view>
    {
        return {};
        // auto starting_fut = common::create_coro_future(coro_, is_stopped).share();
        // auto raw_to_delim_raw_fut = raw_to_delim_raw_converter_.create_future(starting_fut, writers_);
        // auto struct_deser_fut = struct_deserializer_.create_future(starting_fut, writers_);
        // auto proto_converter_fut = struct_proto_converter_.create_future(struct_deser_fut, writers_);
        // auto proto_deser_fut = proto_serializer_.create_future(proto_converter_fut, writers_);
        // auto proto_delim_deser_fut = proto_delim_serializer_.create_future(proto_converter_fut, writers_);

        // if (is_stopped)
        // {
        //     spdlog::debug("Task Workflow: Shutting down all data writers...");
        // }

        // auto make_writer_future = [&](auto& writer)
        // {
        //     using enum process::DataConvertOptions;
        //     const auto convert_mode = writer.get_convert_mode();
        //     if constexpr (std::remove_cvref_t<decltype(writer)>::IsStructType)
        //     {
        //         return writer.write(struct_deser_fut);
        //     }
        //     else
        //     {
        //         switch (convert_mode)
        //         {
        //             case raw:
        //                 return writer.write(starting_fut);
        //             case raw_frame:
        //                 return writer.write(raw_to_delim_raw_fut);
        //             case proto:
        //                 return writer.write(proto_deser_fut);
        //             case proto_frame:
        //                 return writer.write(proto_delim_deser_fut);
        //             default:
        //                 return boost::unique_future<std::optional<std::size_t>>{};
        //         }
        //     }
        // };

        // writers_.write_with(make_writer_future);
        // writers_.wait_for_finished();
        // if (is_stopped)
        // {
        //     spdlog::info("All data consumers have been finished");
        // }
        // return {};
    }

    auto TaskDiagram::run_workflow([[maybe_unused]] bool is_stopped) -> std::expected<void, std::string_view>
    {

        // asio::co_spawn(
        //     io_context_, main_task_diagram_coro_.async_resume(is_stopped, asio::use_awaitable), asio::use_future)
        //     .get();
        return {};
    }

    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    // auto TaskDiagram::generate_starting_coro(asio::any_io_executor /*unused*/) -> StartingCoroType
    // {
    //     while (true)
    //     {
    //         auto data = std::string_view{ binary_data_.data() };
    //         auto is_terminated = co_yield (data);
    //         if (is_terminated)
    //         {
    //             spdlog::debug("Shutting down starting coroutine");
    //             co_return;
    //         }
    //     }
    // }

    void TaskDiagram::reset()
    {
        writers_.reset();
        binary_data_.clear();
    }

    void TaskDiagram::set_output_filenames(const std::vector<std::string>& filenames)
    {
        writers_.set_output_filenames(filenames);
    }
} // namespace srs::workflow
