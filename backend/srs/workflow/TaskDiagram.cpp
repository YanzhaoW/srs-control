#include "srs/workflow/TaskDiagram.hpp"
#include "srs/Application.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include "srs/writers/BinaryFileWriter.hpp"
#include "srs/writers/JsonWriter.hpp"
#include "srs/writers/RootFileWriter.hpp"
#include "srs/writers/UDPWriter.hpp"
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
#include <memory>
#include <oneapi/tbb/concurrent_queue.h>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace srs::workflow
{
    TaskDiagram::TaskDiagram(Handler* data_processor, asio::thread_pool& thread_pool)
        : raw_to_delim_raw_converter_{ thread_pool }
        , struct_deserializer_{ thread_pool }
        , struct_proto_converter_{ thread_pool }
        , proto_serializer_{ thread_pool }
        , proto_delim_serializer_{ thread_pool }
        , io_context_{ thread_pool.get_executor() }
        , writers_{ data_processor }
    {
        coro_ = generate_starting_coro(thread_pool.get_executor());
        common::coro_sync_start(coro_, false, asio::use_awaitable);

        coro_chain_ = [this](asio::any_io_executor /*unused*/) -> asio::experimental::coro<std::size_t(bool)>
        {
            const auto& conversion_req_map = writers_.generate_conversion_req_map();
            spdlog::debug("Conversion requirements map: \n\t {}",
                          fmt::join(conversion_req_map | std::views::transform(
                                                             [](const auto conv_req) -> std::string
                                                             {
                                                                 return fmt::format(
                                                                     "conversion \t {} \t\t required: {}",
                                                                     magic_enum::enum_name(conv_req.first),
                                                                     conv_req.second);
                                                             }),
                                    "\n\t"));
            const auto& binary_writers = writers_.get_binary_writers();
            auto raw_to_delim_coro = std::unique_ptr<decltype(raw_to_delim_raw_converter_)::CoroType>();
            if (conversion_req_map.at(raw_frame))
            {
                raw_to_delim_coro = common::make_unique_coro(raw_to_delim_raw_converter_, io_context_);
            }
            // auto raw_to_delim_coro = raw_to_delim_raw_converter_.extract_coro();
            auto is_terminated = co_yield 0;
            using process::DataConvertOptions;
            for (auto output = std::size_t{ 0 }; not is_terminated; is_terminated = co_yield output)
            {
                if (raw_to_delim_coro != nullptr)
                {
                    auto raw_data_view = co_await raw_to_delim_coro->async_resume(binary_data_.data(), asio::deferred);
                    for (const auto& [_, writer] : binary_writers)
                    {
                        output = (co_await writer->get_coro().async_resume(raw_data_view, asio::deferred)).value_or(0);
                    }
                }
            }

            if (raw_to_delim_coro != nullptr)
            {
                auto raw_data_view =
                    co_await raw_to_delim_coro->async_resume(std::optional<std::string_view>{}, asio::deferred);
                for (const auto& [_, writer] : binary_writers)
                {
                    co_await writer->get_coro().async_resume(raw_data_view, asio::deferred);
                }
            }

            spdlog::debug("Task Workflow: Workflow coroutine has existed");
        }(thread_pool.get_executor());
    }

    void TaskDiagram::init() { common::coro_sync_start(coro_chain_, false, asio::use_awaitable); }

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
            total_read_data_bytes_ += binary_data_.data().size();
            // auto res = run_processes(false);
            auto res = run_workflow(false);
            if (not res.has_value())
            {
                throw std::runtime_error(fmt::format("{}", res.error()));
            }
        }
        else
        {
            total_drop_data_bytes_ += binary_data_.data().size();
        }
        return pop_res;
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
            spdlog::debug("Task Workflow: Shutting down all data writers...");
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
                        return boost::unique_future<std::optional<std::size_t>>{};
                }
            }
        };

        writers_.write_with(make_writer_future);
        writers_.wait_for_finished();
        if (is_stopped)
        {
            spdlog::info("All data consumers have been finished");
        }
        return {};
    }

    auto TaskDiagram::run_workflow(bool is_stopped) -> std::expected<void, std::string_view>
    {

        asio::co_spawn(io_context_, coro_chain_.async_resume(is_stopped, asio::use_awaitable), asio::use_future).get();
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
                spdlog::debug("Shutting down starting coroutine");
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
