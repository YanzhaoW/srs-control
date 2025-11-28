#pragma once

#include "srs/converters/DataConverterBase.hpp"
#include <concepts>
#include <cstddef>

namespace srs::writer
{

    namespace internal
    {

        template <typename T>
        class Dumpy
        {
          public:
            auto get_data_view(std::size_t /*unused*/) -> T { return T{}; }
        };

    } // namespace internal

    template <typename T>
    concept WritableFile = requires(T writer, const internal::Dumpy<typename T::InputType>& converter) {
        typename T::InputType;
        typename T::OutputType;
        std::derived_from<T, process::BaseTask<typename T::InputType, typename T::OutputType>>;
        { writer(converter, 0) } -> std::same_as<typename T::OutputType>;
        { writer(converter) } -> std::same_as<typename T::OutputType>;
        { writer.get_data() } -> std::same_as<typename T::OutputType>;
        { writer.get_data(0) } -> std::same_as<typename T::OutputType>;
        not std::copyable<T>;
        std::movable<T>;
    };
} // namespace srs::writer
