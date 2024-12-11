#pragma once

#include <zpp_bits.h>

#include <srs/Application.hpp>
#include <srs/converters/DataConverterBase.hpp>

namespace srs::process
{
    class StructDeserializer : public DataConverterBase<std::string_view, const StructData*>
    {
      public:
        using DataElementType = std::bitset<common::HIT_DATA_BIT_LENGTH>;
        using ReceiveDataSquence = std::vector<DataElementType>;
        static constexpr auto ConverterOption = std::array{ structure };

        explicit StructDeserializer(asio::thread_pool& thread_pool)
            : DataConverterBase{ generate_coro(thread_pool.get_executor()) }
        {
        }

        [[nodiscard]] auto data() const -> const auto& { return output_data_; }

      private:
        ReceiveDataSquence receive_raw_data_;
        StructData output_data_;

        void reset() { reset_struct_data(output_data_); }

        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        auto generate_coro(asio::any_io_executor /*unused*/) -> CoroType;
        auto convert(std::string_view binary_data) -> std::size_t;
        void translate_raw_data(StructData& struct_data);
        void byte_reverse_data_sq();
        static auto check_is_hit(const DataElementType& element) -> bool
        {
            return element.test(common::FLAG_BIT_POSITION);
        }
    };

} // namespace srs::process
