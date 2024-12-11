#include <srs/workflow/TaskDiagram.hpp>
#include <srs/writers/BinaryFileWriter.hpp>
#include <srs/writers/JsonWriter.hpp>
#include <srs/writers/RootFileWriter.hpp>
#include <srs/writers/UDPWriter.hpp>

namespace srs::workflow
{
    TaskDiagram::TaskDiagram(Handler* data_processor, asio::thread_pool& thread_pool)
        : raw_to_delim_raw_converter_{ thread_pool }
        , struct_deserializer_{ thread_pool }
        , struct_proto_converter_{ thread_pool }
        , proto_serializer_{ thread_pool }
        , proto_delim_serializer_{ thread_pool }
        , writers_{ data_processor }
    {
        coro_ = generate_starting_coro(thread_pool.get_executor());
        common::coro_sync_start(coro_, false, asio::use_awaitable);
    }

    auto TaskDiagram::analysis_one(tbb::concurrent_bounded_queue<process::SerializableMsgBuffer>& data_queue,
                                   bool is_blocking) -> bool
    {
        auto pop_res = true;
        reset();
        if (is_blocking)
        {
            data_queue.pop(binary_data_);
        }
        else
        {
            pop_res = data_queue.try_pop(binary_data_);
        }
        if (pop_res)
        {
            auto res = run_processes(false);
            if (not res.has_value())
            {
                throw std::runtime_error(fmt::format("{}", res.error()));
            }
        }
        return pop_res;
    }

    TaskDiagram::~TaskDiagram()
    {
        spdlog::debug("Closing analysis task workflows ...");
        reset();
        auto res = run_processes(true);
        if (not res.has_value())
        {
            spdlog::error("{}", res.error());
        }
    }

    auto TaskDiagram::run_processes(bool is_stopped) -> std::expected<void, std::string_view>
    {
        auto starting_fut = common::create_coro_future(coro_, is_stopped).share();
        auto raw_to_delim_raw_fut = raw_to_delim_raw_converter_.create_future(starting_fut, writers_);
        auto struct_deser_fut = struct_deserializer_.create_future(starting_fut, writers_);
        auto proto_converter_fut = struct_proto_converter_.create_future(struct_deser_fut, writers_);
        auto proto_deser_fut = proto_serializer_.create_future(proto_converter_fut, writers_);
        auto proto_delim_deser_fut = proto_delim_serializer_.create_future(proto_converter_fut, writers_);

        if (is_stopped)
        {
            spdlog::debug("Shutting down all data writers...");
        }

        auto make_writer_future = [&](auto& writer)
        {
            const auto convert_mode = writer.get_convert_mode();
            if constexpr (std::remove_cvref_t<decltype(writer)>::IsStructType)
            {
                return writer.write(struct_deser_fut);
            }
            else
            {
                switch (convert_mode)
                {
                    case raw:
                        return writer.write(starting_fut);
                    case raw_frame:
                        return writer.write(raw_to_delim_raw_fut);
                    case proto:
                        return writer.write(proto_deser_fut);
                    case proto_frame:
                        return writer.write(proto_delim_deser_fut);
                    default:
                        return boost::unique_future<std::optional<int>>{};
                }
            }
        };

        writers_.write_with(make_writer_future);
        writers_.wait_for_finished();
        if (is_stopped)
        {
            spdlog::info("All data consumers are finished.");
        }
        return {};
    }

    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    auto TaskDiagram::generate_starting_coro(asio::any_io_executor /*unused*/) -> StartingCoroType
    {
        while (true)
        {
            auto data = std::string_view{ binary_data_.data() };
            auto is_terminated = co_yield (data);
            if (is_terminated)
            {
                spdlog::debug("Shutting down starting coroutine.");
                co_return;
            }
        }
    }

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
