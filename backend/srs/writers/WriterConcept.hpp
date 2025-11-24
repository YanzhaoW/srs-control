#pragma once

#include <concepts>
#include <cstddef>
#include <string_view>
#include <utility>

namespace srs::process
{
    class Raw2DelimRawConverter;
    class StructDeserializer;
    class Struct2ProtoConverter;
    class ProtoSerializer;
    class ProtoDelimSerializer;
}; // namespace srs::process

namespace srs::writer
{
    template <typename T>
    concept WritableFile = requires(T writer) {
        { writer.get_name() } -> std::same_as<std::string_view>;
        { writer.write(std::declval<process::Raw2DelimRawConverter>(), std::size_t{}) } -> std::same_as<void>;
        { writer.write(std::declval<process::StructDeserializer>(), std::size_t{}) } -> std::same_as<void>;
        { writer.write(std::declval<process::Struct2ProtoConverter>(), std::size_t{}) } -> std::same_as<void>;
        { writer.write(std::declval<process::ProtoSerializer>(), std::size_t{}) } -> std::same_as<void>;
        { writer.write(std::declval<process::ProtoDelimSerializer>(), std::size_t{}) } -> std::same_as<void>;
        T{ std::string_view{}, std::size_t{} };
    };
} // namespace srs::writer
