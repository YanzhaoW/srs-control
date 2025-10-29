#pragma once

#include "srs/Application.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/ProtoDelimSerializer.hpp"
#include "srs/converters/ProtoSerializer.hpp"
#include "srs/converters/RawToDelimRawConveter.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/converters/StructDeserializer.hpp"
#include "srs/converters/StructToProtoConverter.hpp"
#include "srs/writers/DataWriter.hpp"
#include <atomic>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cstdint>
#include <expected>
#include <oneapi/tbb/concurrent_queue.h>
#include <string>
#include <string_view>
#include <tbb/concurrent_queue.h>
#include <vector>

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

        [[nodiscard]] auto get_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }

        auto get_struct_data() -> const auto& { return struct_deserializer_.data(); }

      private:
        process::SerializableMsgBuffer binary_data_;
        process::Raw2DelimRawConverter raw_to_delim_raw_converter_;
        process::StructDeserializer struct_deserializer_;
        process::Struct2ProtoConverter struct_proto_converter_;
        process::ProtoSerializer proto_serializer_;
        process::ProtoDelimSerializer proto_delim_serializer_;

        std::atomic<uint64_t> total_read_data_bytes_ = 0;

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
