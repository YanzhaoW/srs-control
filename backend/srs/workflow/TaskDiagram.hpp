#pragma once

#include <expected>
#include <tbb/concurrent_queue.h>

#include <srs/converters/ProtoDelimSerializer.hpp>
#include <srs/converters/ProtoSerializer.hpp>
#include <srs/converters/RawToDelimRawConveter.hpp>
#include <srs/converters/SerializableBuffer.hpp>
#include <srs/converters/StructToProtoConverter.hpp>
#include <srs/utils/ValidData.hpp>

namespace srs::workflow
{
    class TaskDiagram
    {
      public:
        explicit TaskDiagram(Handler* data_processor, asio::thread_pool& thread_pool);

        // rule of 5
        ~TaskDiagram();
        TaskDiagram(const TaskDiagram&) = delete;
        TaskDiagram(TaskDiagram&&) = delete;
        TaskDiagram& operator=(const TaskDiagram&) = delete;
        TaskDiagram& operator=(TaskDiagram&&) = delete;

        using enum process::DataConvertOptions;
        using StartingCoroType = asio::experimental::coro<std::string_view(bool)>;

        auto analysis_one(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue, bool is_blocking)
            -> bool;
        void set_output_filenames(const std::vector<std::string>& filenames);
        void reset();
        // void stop() { run_processes(true); }

        // Getters:
        template <process::DataConvertOptions option>
        auto get_data() -> std::string_view;

        auto get_struct_data() -> const auto& { return struct_deserializer_.data(); }

      private:
        process::SerializableMsgBuffer binary_data_;
        process::Raw2DelimRawConverter raw_to_delim_raw_converter_;
        process::StructDeserializer struct_deserializer_;
        process::Struct2ProtoConverter struct_proto_converter_;
        process::ProtoSerializer proto_serializer_;
        process::ProtoDelimSerializer proto_delim_serializer_;

        StartingCoroType coro_;

        writer::Manager writers_;

        auto generate_starting_coro(asio::any_io_executor /*unused*/) -> StartingCoroType;
        [[maybe_unused]] auto run_processes(bool is_stopped) -> std::expected<void, std::string_view>;
    };

    template <process::DataConvertOptions option>
    auto TaskDiagram::get_data() -> std::string_view
    {
        if constexpr (option == raw)
        {
            return binary_data_.data();
        }
        else if constexpr (option == proto)
        {
            return proto_serializer_.data();
        }
        else
        {
            static_assert(false, "Cannot get the data from this option!");
        }
    };
} // namespace srs::workflow
