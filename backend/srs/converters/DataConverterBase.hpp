#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/writers/DataWriterOptions.hpp"
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/thread/future.hpp>
#include <cstddef>
#include <expected>
#include <fmt/ranges.h>
#include <gsl/gsl-lite.hpp>
#include <string>
#include <string_view>

namespace srs::process
{

    template <typename Input, typename Output>
    class BaseTask
    {
      public:
        using InputType = Input;
        using OutputType = Output;

        using enum DataConvertOptions;

        using RunResult = std::expected<OutputType, std::string_view>;

        explicit BaseTask(std::string_view name, DataConvertOptions prev_convert, std::size_t n_lines = 1)
            : previous_conversion_{ prev_convert }
            , name_{ name }
            , n_lines_{ n_lines }
        {
        }

        [[nodiscard]] auto get_n_lines() const -> std::size_t { return n_lines_; }
        [[nodiscard]] auto get_required_conversion() const -> DataConvertOptions { return previous_conversion_; }
        [[nodiscard]] auto get_name() const -> std::string_view { return name_; };
        [[nodiscard]] auto get_name_str() const -> std::string { return std::string{ name_ }; };

      private:
        process::DataConvertOptions previous_conversion_;
        std::string name_ = "empty";
        std::size_t n_lines_ = 1;
    };

    template <DataConvertOptions Conversion, typename Input, typename Output>
    class ConverterTask : public BaseTask<Input, Output>
    {
      public:
        constexpr static auto converter_type = Conversion;
        explicit ConverterTask(std::string_view name, DataConvertOptions prev_convert, std::size_t n_lines = 1)
            : BaseTask<Input, Output>{ name, prev_convert, n_lines }
        {
        }
    };

    template <DataWriterOption writer, typename Input, typename Output>
    class WriterTask : public BaseTask<Input, Output>
    {
      public:
        constexpr static auto writer_type = writer;
        explicit WriterTask(std::string_view name, DataConvertOptions prev_convert, std::size_t n_lines = 1)
            : BaseTask<Input, Output>{ name, prev_convert, n_lines }
        {
        }
    };
} // namespace srs::process
