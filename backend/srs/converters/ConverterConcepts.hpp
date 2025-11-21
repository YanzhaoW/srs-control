#pragma once

#include <concepts>

namespace srs::process
{
    template <typename T>
    concept ConvertibleTask = requires(T converter) {
        typename T::InputType;
        typename T::OutputType;
        typename T::ConverterOption;
        { converter.get_data_view() } -> std::same_as<typename T::OutputType>;
    };
} // namespace srs::process
