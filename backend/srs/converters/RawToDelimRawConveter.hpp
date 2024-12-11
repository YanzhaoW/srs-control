#pragma once

#include <zpp_bits.h>

#include <srs/converters/DataConverterBase.hpp>

namespace srs::process
{
    class Raw2DelimRawConverter : public DataConverterBase<std::string_view, std::string_view>
    {
      public:
        explicit Raw2DelimRawConverter(asio::thread_pool& thread_pool)
            : DataConverterBase{ generate_coro(thread_pool.get_executor()) }
        {
        }

        using SizeType = common::RawDelimSizeType;
        static constexpr auto ConverterOption = std::array{ raw_frame };

        [[nodiscard]] auto data() const -> OutputType
        {
            return std::string_view{ output_data_.data(), output_data_.size() };
        }

      private:
        std::vector<char> output_data_;

        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        auto generate_coro(asio::any_io_executor /*unused*/) -> CoroType
        {
            InputType temp_data{};
            while (true)
            {
                if (not temp_data.empty())
                {
                    output_data_.clear();
                    convert(temp_data, output_data_);
                }
                auto output_temp = std::string_view{ output_data_.data(), output_data_.size() };
                auto data = co_yield (output_temp);
                if (data.has_value())
                {
                    temp_data = data.value();
                }
                else
                {
                    spdlog::debug("Shutting down Raw2DelimRaw converter.");
                    co_return;
                }
            }
        }

        static void convert(std::string_view input, std::vector<char>& output)
        {
            auto size = static_cast<SizeType>(input.size());
            output.reserve(size + sizeof(size));
            auto deserialize_to = zpp::bits::out{ output, zpp::bits::append{}, zpp::bits::endian::big{} };
            deserialize_to(size, zpp::bits::unsized(input)).or_throw();
        }
    };
} // namespace srs
