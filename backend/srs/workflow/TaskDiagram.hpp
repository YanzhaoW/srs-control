#pragma once

#include "srs/Application.hpp"
#include "srs/converters/ProtoDelimSerializer.hpp"
#include "srs/converters/ProtoSerializer.hpp"
#include "srs/converters/RawToDelimRawConveter.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/converters/StructDeserializer.hpp"
#include "srs/converters/StructToProtoConverter.hpp"
#include "srs/writers/DataWriter.hpp"
#include <algorithm>
#include <atomic>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <oneapi/tbb/concurrent_queue.h>
#include <optional>
#include <string>
#include <string_view>
#include <taskflow/core/executor.hpp>
#include <taskflow/core/task.hpp>
#include <taskflow/core/taskflow.hpp>
#include <tbb/concurrent_queue.h>
#include <vector>

namespace srs::workflow
{
    class TaskDiagram
    {
      public:
        explicit TaskDiagram(Handler* data_processor, asio::thread_pool& thread_pool, std::size_t n_lines = 1);

        // rule of 5
        ~TaskDiagram();
        TaskDiagram(const TaskDiagram&) = delete;
        TaskDiagram(TaskDiagram&&) = delete;
        TaskDiagram& operator=(const TaskDiagram&) = delete;
        TaskDiagram& operator=(TaskDiagram&&) = delete;

        // using StartingCoroType = asio::experimental::coro<std::string_view(bool)>;
        using InputType = bool;
        using OutputType = std::size_t;
        // using CoroType = asio::experimental::coro<std::size_t(bool)>;

        // auto analysis_one(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue, bool
        // is_blocking)
        //     -> bool;
        auto analysis_one(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue) -> bool;
        void set_output_filenames(const std::vector<std::string>& filenames);
        void reset();
        void construct_taskflow_and_run(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
                                const std::atomic<bool>& is_stopped);
        void run_task(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
                      std::size_t line_number);
        void run();
        auto is_taskflow_abort_ready() const -> bool;
        // void stop() { run_processes(true); }

        // Getters:
        // template <process::DataConvertOptions option>
        // auto get_data() -> std::string_view;
        [[nodiscard]] auto get_data_view(std::size_t line_number) const -> std::string_view
        {
            return raw_data_[line_number].data();
        }

        [[nodiscard]] auto get_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }
        [[nodiscard]] auto get_n_lines() const -> std::size_t { return n_lines_; }

        auto get_struct_data() -> const auto* { return struct_deserializer_converter_.get_data_view(0); }
        auto get_executor() -> asio::any_io_executor { return io_context_; }
        // auto generate_coro() -> asio::experimental::coro<std::size_t(bool)>;

      private:
        std::size_t n_lines_ = 1;
        tf::Executor tf_executor_;
        tf::Taskflow main_taskflow_;
        std::vector<tf::Taskflow> taskflow_lines_;
        std::vector<std::atomic<bool>> is_pipeline_stopped_;
        std::vector<process::SerializableMsgBuffer> raw_data_;
        process::SerializableMsgBuffer binary_data_;

        process::Raw2DelimRawConverter raw_to_delim_raw_converter_;
        process::StructDeserializer struct_deserializer_converter_;
        process::Struct2ProtoConverter struct_to_proto_converter_;
        process::ProtoSerializer proto_serializer_converter_;
        process::ProtoDelimSerializer proto_delim_serializer_converter_;

        std::atomic<uint64_t> total_read_data_bytes_ = 0;

        asio::any_io_executor io_context_ = nullptr;
        // StartingCoroType coro_;
        // asio::experimental::coro<std::size_t(bool)> coro_chain_;
        // CoroType main_task_diagram_coro_;

        writer::Manager writers_;

        // auto generate_starting_coro(asio::any_io_executor /*unused*/) -> StartingCoroType;
        void construct_taskflow_line(tf::Taskflow& taskflow, std::size_t line_number);
        [[maybe_unused]] auto run_processes(bool is_stopped) -> std::expected<void, std::string_view>;
        [[maybe_unused]] auto run_workflow(bool is_stopped) -> std::expected<void, std::string_view>;

        // TODO: Add converter concept here
        auto create_task(auto& converter, tf::Taskflow& taskflow, std::size_t line_number) -> std::optional<tf::Task>
        {
            auto is_required = std::ranges::all_of(converter.DataConvertOptions,
                                                   [this](auto converter_option)
                                                   { return writers_.is_convert_required(converter_option); });
            if (not is_required)
            {
                return {};
            }
            return taskflow.emplace([this, line_number, &converter]() { converter.run_task(*this, line_number); })
                .name(converter.get_name_str());
        }
    };

    // template <process::DataConvertOptions option>
    // auto TaskDiagram::get_data() -> std::string_view
    // {
    //     using enum process::DataConvertOptions;
    //     if constexpr (option == raw)
    //     {
    //         return binary_data_.data();
    //     }
    //     else if constexpr (option == proto)
    //     {
    //         return proto_serializer_.data();
    //     }
    //     else
    //     {
    //         static_assert(false, "Cannot get the data from this option!");
    //     }
    // };
} // namespace srs::workflow
