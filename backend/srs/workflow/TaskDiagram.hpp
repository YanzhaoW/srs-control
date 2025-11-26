#pragma once

#include "srs/Application.hpp"
#include "srs/converters/ProtoSerializer.hpp"
#include "srs/converters/RawToDelimRawConveter.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/converters/StructDeserializer.hpp"
#include "srs/converters/StructToProtoConverter.hpp"
#include "srs/writers/DataWriter.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <optional>
#include <string_view>
#include <taskflow/core/executor.hpp>
#include <taskflow/core/task.hpp>
#include <taskflow/core/taskflow.hpp>
#include <vector>

#ifdef USE_ONEAPI_TBB
#include <oneapi/tbb/concurrent_queue.h>
#else
#include <tbb/concurrent_queue.h>
#endif

namespace srs::workflow
{
    class TaskDiagram
    {
      public:
        explicit TaskDiagram(Handler* data_processor, std::size_t n_lines = 1);

        using InputType = bool;
        using OutputType = std::size_t;
        void construct_taskflow_and_run(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
                                        const std::atomic<bool>& is_stopped);
        void run_task(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
                      std::size_t line_number);
        void run();
        auto is_taskflow_abort_ready() const -> bool;
        [[nodiscard]] auto get_data_view(std::size_t line_number) const -> std::string_view
        {
            return raw_data_[line_number].data();
        }

        [[nodiscard]] auto get_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }
        [[nodiscard]] auto get_n_lines() const -> std::size_t { return n_lines_; }

        auto get_struct_data() -> const auto* { return struct_deserializer_converter_.get_data_view(0); }
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
        gsl::not_null<writer::Manager*> writers_;

        void construct_taskflow_line(tf::Taskflow& taskflow, std::size_t line_number);

        // TODO: Add converter concept here
        auto create_task(auto& converter, tf::Taskflow& taskflow, std::size_t line_number) -> std::optional<tf::Task>
        {
            auto is_required = std::ranges::all_of(converter.DataConvertOptions,
                                                   [this](auto converter_option)
                                                   { return writers_->is_convert_required(converter_option); });
            if (not is_required)
            {
                return {};
            }
            return taskflow.emplace([this, line_number, &converter]() { converter.run_task(*this, line_number); })
                .name(converter.get_name_str());
        }
    };
} // namespace srs::workflow
