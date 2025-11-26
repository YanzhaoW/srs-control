#pragma once

#include "DataConverterBase.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/data/message.pb.h"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <google/protobuf/message_lite.h>
#include <spdlog/spdlog.h>
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
        using Base = ConverterTask<Conversion, const proto::Data*, std::string_view>;

        ProtoSerializerBase(const ProtoSerializerBase&) = delete;
        ProtoSerializerBase(ProtoSerializerBase&&) = delete;
        ProtoSerializerBase& operator=(const ProtoSerializerBase&) = delete;
        ProtoSerializerBase& operator=(ProtoSerializerBase&&) = delete;
        ~ProtoSerializerBase() { spdlog::debug("Shutting down {:?} serializer.", name_); }

        [[nodiscard]] auto get_data_view(std::size_t line_num) const -> std::string_view
        {
            assert(line_num < Base::get_n_lines());
            return output_data_[line_num];
        }
        void run_task(const auto& prev_data_converter, std::size_t line_num)
        {
            assert(line_num < Base::get_n_lines());
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

    class protobuf_deserializer_converter
    {
      public:
        auto operator()(const proto::Data& proto_data, std::string& output_data) -> int;
    };

    class ProtoSerializer : public ProtoSerializerBase<protobuf_deserializer_converter, DataConvertOptions::proto>
    {
      public:
        explicit ProtoSerializer(std::size_t n_lines)
            : ProtoSerializerBase{ "ProtoSerializer", protobuf_deserializer_converter{}, n_lines }
        {
        }
    };

    class protobuf_delim_deserializer_converter
    {
      public:
        auto operator()(const proto::Data& proto_data, std::string& output_data) -> int;
    };

    class ProtoDelimSerializer
        : public ProtoSerializerBase<protobuf_delim_deserializer_converter, DataConvertOptions::proto_frame>
    {
      public:
        explicit ProtoDelimSerializer(std::size_t n_lines)
            : ProtoSerializerBase{ "ProtoSerializer(delim)", protobuf_delim_deserializer_converter{}, n_lines }
        {
        }
    };
} // namespace srs::process
