#pragma once

#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <array>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>
#include <zpp_bits.h>

#include <srs/converters/DataConverterBase.hpp>

namespace srs::process
{
    class Raw2DelimRawConverter : public DataConverterBase<std::string_view, std::string_view>
    {
      public:
        explicit Raw2DelimRawConverter(asio::thread_pool& thread_pool)
            : DataConverterBase{ thread_pool }
        {
        }

        using SizeType = common::RawDelimSizeType;
        static constexpr auto ConverterOption = std::array{ raw_frame };

        [[nodiscard]] auto data() const -> OutputType
        {
            return std::string_view{ output_data_.data(), output_data_.size() };
        }

        auto generate_coro() -> CoroType
        {
            spdlog::trace("Process: Starting Raw2DelimRaw converter.");
            auto binary_data = co_yield OutputType{};

            while (true)
            {
                if (not binary_data.has_value())
                {
                    break;
                }
                output_data_.clear();
                convert(binary_data.value(), output_data_);

                binary_data = co_yield std::string_view{ output_data_.data(), output_data_.size() };
            }
            spdlog::debug("Process: Shutting down Raw2DelimRaw converter.");
            co_return;
        }

      private:
        std::vector<char> output_data_;

        static void convert(std::string_view input, std::vector<char>& output)
        {
            auto size = static_cast<SizeType>(input.size());
            output.reserve(size + sizeof(size));
            auto deserialize_to = zpp::bits::out{ output, zpp::bits::append{}, zpp::bits::endian::big{} };
            deserialize_to(size, zpp::bits::unsized(input)).or_throw();
        }
    };
} // namespace srs::process
