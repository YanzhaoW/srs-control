#pragma once

#include "srs/utils/CommonAlias.hpp" // IWYU pragma: keep
#include <asio/any_io_executor.hpp>
#include <concepts>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace srs
{
    template <typename T>
    concept Deserializer = requires(T) {
        typename T::OutputType;
        typename T::InputType;
    };

    template <typename T>
    concept StreamableOutput = requires(T obj) { std::declval<std::ofstream>() << obj; };

    template <typename T>
    concept StreamableDeserializer = Deserializer<T> and StreamableOutput<typename T::OutputType>;

    template <typename T>
    concept RangedData = requires(T obj) {
        requires std::contiguous_iterator<decltype(obj.begin())>;
        requires std::contiguous_iterator<decltype(obj.end())>;
    };

    template <typename DataType, typename TaskType>
    concept OutputCompatible = requires(DataType, TaskType task) {
        std::is_same_v<DataType, std::remove_cvref_t<decltype(task.get_output_data())>>;
    };

    template <typename Converter, typename OutputType>
    concept OutputTo = requires(Converter converter) {
        std::is_const_v<OutputType>;
        std::is_reference_v<OutputType>;
        { converter(0) } -> std::same_as<OutputType>;
    };

    template <typename T>
    concept ConverterType = requires { T::converter_type; };

    template <typename T>
    concept SinkType = requires { T::writer_type; };

    template <typename T>
    struct is_optional_type : std::false_type
    {
    };

    template <typename T>
    struct is_optional_type<std::optional<T>> : std::true_type
    {
    };

    template <typename T>
    struct is_vector_type : std::false_type
    {
    };

    template <typename T>
    struct is_vector_type<std::vector<T>> : std::true_type
    {
    };

    template <typename T>
    struct is_map_type : std::false_type
    {
    };

    template <typename T, typename U>
    struct is_map_type<std::map<T, U>> : std::true_type
    {
    };

    template <typename T>
    struct remove_optional
    {
        using type = T;
    };

    template <typename T>
    struct remove_optional<std::optional<T>>
    {
        using type = T;
    };

    template <typename T>
    using remove_optional_t = remove_optional<T>::type;

}; // namespace srs
