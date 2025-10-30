#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/thread/future.hpp>
#include <cstddef>
#include <fmt/format.h>
#include <fstream>
#include <ios>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>

#include <srs/utils/CommonConcepts.hpp>
#include <srs/workflow/TaskDiagram.hpp>
#include <srs/writers/DataWriterOptions.hpp>
#include <string_view>
#include <utility>

namespace srs::writer
{
    class BinaryFile
    {
      public:
        using InputType = std::string_view;
        using OutputType = std::size_t;
        using CoroType = asio::experimental::coro<OutputType(std::optional<InputType>)>;
        using InputFuture = boost::shared_future<std::optional<InputType>>;
        using OutputFuture = boost::unique_future<std::optional<OutputType>>;
        static constexpr auto IsStructType = false;

        explicit BinaryFile(asio::thread_pool& thread_pool,
                            const std::string& filename,
                            process::DataConvertOptions deser_mode)
            : convert_mode_{ deser_mode }
            , file_name_{ filename }
            , ofstream_{ filename, std::ios::trunc }
        {
            if (not ofstream_.is_open())
            {
                throw std::runtime_error(fmt::format("Filename {:?} cannot be open!", filename));
            }
            coro_ = generate_coro(thread_pool.get_executor());
            common::coro_sync_start(coro_, std::optional<InputType>{}, asio::use_awaitable);
        }
        auto write(auto pre_future) -> OutputFuture { return common::create_coro_future(coro_, pre_future); }
        auto get_convert_mode() const -> process::DataConvertOptions { return convert_mode_; }
        void close() { ofstream_.close(); }
        auto get_coro() -> auto& { return coro_; }

      private:
        process::DataConvertOptions convert_mode_ = process::DataConvertOptions::none;
        std::string file_name_;
        std::ofstream ofstream_;
        CoroType coro_;

        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        auto generate_coro(asio::any_io_executor /*unused*/) -> CoroType
        {
            auto total_size = std::size_t{};
            auto write_msg = std::string_view{};
            while (true)
            {
                if (not write_msg.empty())
                {
                    // spdlog::trace("writer size: {}", write_msg.size());
                    total_size += write_msg.size();
                    ofstream_ << write_msg;
                }
                auto msg = co_yield write_msg.size();

                if (msg.has_value())
                {
                    write_msg = msg.value();
                }
                else
                {
                    close();
                    spdlog::info(
                        "Binary file {} is closed successfully with {}B written in total.", file_name_, total_size);
                    co_return;
                }
            }
        }
    };
} // namespace srs::writer
