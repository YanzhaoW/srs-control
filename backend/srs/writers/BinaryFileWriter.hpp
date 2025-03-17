#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>

#include <srs/utils/CommonConcepts.hpp>
#include <srs/workflow/TaskDiagram.hpp>
#include <srs/writers/DataWriterOptions.hpp>

namespace srs::writer
{
    class BinaryFile
    {
      public:
        using InputType = std::string_view;
        using OutputType = int;
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

      private:
        process::DataConvertOptions convert_mode_ = process::DataConvertOptions::none;
        std::string file_name_;
        std::ofstream ofstream_;
        CoroType coro_;

        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        auto generate_coro(asio::any_io_executor /*unused*/) -> CoroType
        {
            auto write_msg = std::string_view{};
            while (true)
            {
                if (not write_msg.empty())
                {
                    ofstream_ << write_msg;
                }
                auto msg = co_yield static_cast<int>(write_msg.size());

                if (msg.has_value())
                {
                    write_msg = msg.value();
                }
                else
                {
                    close();
                    spdlog::info("Binary file {} is closed successfully", file_name_);
                    co_return;
                }
            }
        }
    };
} // namespace srs::writer
