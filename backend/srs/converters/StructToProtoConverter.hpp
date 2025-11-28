#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/data/message.pb.h"
#include "srs/utils/CommonConcepts.hpp"
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cassert>
#include <cstddef>
#include <string_view>
#include <vector>

namespace srs::process
{
    class Struct2ProtoConverter
        : public ConverterTask<DataConvertOptions::structure_to_proto, const StructData*, const proto::Data*>
    {
      public:
        explicit Struct2ProtoConverter(std::size_t n_lines)
            : ConverterTask{ "Struct deserializer", structure, n_lines }
        {
            output_data_.resize(n_lines);
        }

        [[nodiscard]] auto get_data_view(std::size_t line_num) const -> OutputType
        {
            assert(line_num < get_n_lines());
            return &output_data_[line_num];
        }

        auto operator()(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number) -> OutputType
        {
            assert(line_number < get_n_lines());
            auto& output_data = output_data_[line_number];
            output_data.Clear();
            auto input_data = prev_data_converter.get_data_view(line_number);
            convert(*input_data, output_data);
            return get_data_view(line_number);
        }

      private:
        std::vector<proto::Data> output_data_;
        static void convert(const StructData& struct_data, proto::Data& output_data);
    };
} // namespace srs::process
