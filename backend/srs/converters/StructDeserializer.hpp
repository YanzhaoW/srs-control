#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <bitset>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cassert>
#include <cstddef>
#include <expected>
#include <string_view>
#include <vector>
#include <zpp_bits.h>

#include <srs/Application.hpp>
#include <srs/converters/DataConverterBase.hpp>

namespace srs::workflow
{
    class TaskDiagram;
}

namespace srs::process
{
    class StructDeserializer : public ConverterTask<DataConvertOptions::structure, std::string_view, const StructData*>
    {
      public:
        using DataElementType = std::bitset<common::HIT_DATA_BIT_LENGTH>;
        using ReceiveDataSquence = std::vector<DataElementType>;
        explicit StructDeserializer(size_t n_lines = 1);

        [[nodiscard]] auto operator()(std::size_t line_number = 0) const -> OutputType
        {
            assert(line_number < get_n_lines());
            return &output_data_[line_number];
        }

        auto run(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number = 0) -> RunResult
        {
            assert(line_number < get_n_lines());
            auto& output_data = output_data_[line_number];
            auto& raw_body_part_data = raw_body_part_data_[line_number];
            reset_struct_data(output_data);
            raw_body_part_data.clear();
            auto res = convert(prev_data_converter(line_number), output_data, raw_body_part_data);
            return res.transform([line_number, this](auto) { return this->operator()(line_number); });
        }

      private:
        std::vector<ReceiveDataSquence> raw_body_part_data_;
        std::vector<StructData> output_data_;

        static auto convert(std::string_view binary_data, StructData& output, ReceiveDataSquence& body_part_data)
            -> std::expected<std::size_t, std::string_view>;
        static void translate_raw_data(StructData& struct_data, ReceiveDataSquence& receive_raw_data);
        static void byte_reverse_data_sq(ReceiveDataSquence& receive_raw_data);
        static auto check_is_hit(const DataElementType& element) -> bool
        {
            return element.test(common::FLAG_BIT_POSITION);
        }
    };

} // namespace srs::process
