#pragma once

#include "DataConverterBase.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <spdlog/spdlog.h>
#include <srs/data/message.pb.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace srs::process
{
    template <typename Converter, DataConvertOptions Conversion>
    class ProtoSerializerBase : public ConverterTask<Conversion, const proto::Data*, std::string_view>
    {
      public:
        explicit ProtoSerializerBase(std::string name, Converter converter, std::size_t n_lines = 1)
            : ConverterTask<Conversion, const proto::Data*, std::string_view>{ name,
                                                                               DataConvertOptions::structure_to_proto,
                                                                               n_lines }
            , name_{ std::move(name) }
            , converter_{ converter }
        {
            output_data_.resize(n_lines);
        }

        ProtoSerializerBase(const ProtoSerializerBase&) = delete;
        ProtoSerializerBase(ProtoSerializerBase&&) = delete;
        ProtoSerializerBase& operator=(const ProtoSerializerBase&) = delete;
        ProtoSerializerBase& operator=(ProtoSerializerBase&&) = delete;
        ~ProtoSerializerBase() { spdlog::debug("Shutting down {:?} serializer.", name_); }

        [[nodiscard]] auto get_data_view(std::size_t line_num) const -> std::string_view
        {
            assert(line_num < n_lines_);
            return output_data_[line_num];
        }
        void run_task(const auto& prev_data_converter, std::size_t line_num)
        {
            assert(line_num < n_lines_);
            output_data_[line_num].clear();
            const auto* input_data = prev_data_converter.get_data_view(line_num);
            static_assert(std::same_as<decltype(input_data), const proto::Data*>);
            converter_(*input_data, output_data_[line_num]);
        }

      private:
        std::string name_;
        std::vector<std::string> output_data_;
        Converter converter_;
    };

} // namespace srs::process
