#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include "srs/writers/DataWriter.hpp"
#include <algorithm>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/thread/future.hpp>
#include <fmt/ranges.h>
#include <gsl/gsl-lite.hpp>
#include <optional>
#include <type_traits>

namespace srs::process
{
    template <typename Input, typename Output>
    class DataConverterBase
    {
      public:
        using InputType = Input;
        using OutputType = Output;
        using InputFuture = boost::shared_future<std::optional<InputType>>;
        using OutputFuture = boost::shared_future<std::optional<OutputType>>;
        using CoroType = asio::experimental::coro<OutputType(std::optional<InputType>)>;

        using enum DataConvertOptions;

        explicit DataConverterBase(auto coro)
            : coro_(std::move(coro))
        {
            common::coro_sync_start(coro_, std::optional<InputType>{}, asio::use_awaitable);
        }

        explicit DataConverterBase(io_context_type& io_context)
            : io_context_{ &io_context }
        {
        }

        auto extract_coro() -> CoroType { return std::move(coro_); }

        auto create_future(this auto&& self, InputFuture& pre_fut, writer::Manager& writers) -> OutputFuture
        {
            constexpr auto converter_options = std::remove_cvref_t<decltype(self)>::ConverterOption;
            auto is_needed = std::ranges::any_of(
                converter_options, [&writers](auto option) { return writers.is_convert_required(option); });
            return is_needed ? common::create_coro_future(self.coro_, pre_fut) : OutputFuture{};
        }
        auto get_executor() const -> auto { return io_context_->get_executor(); }

      private:
        // gsl::not_null<io_context_type*> io_context_;
        io_context_type* io_context_ = nullptr;
        CoroType coro_;
    };
} // namespace srs::process
