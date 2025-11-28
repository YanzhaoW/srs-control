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
            auto operator()(std::size_t /*unused*/) -> T { return T{}; }
        };

    } // namespace internal

    template <typename T>
    concept WritableFile = requires(T writer, const internal::Dumpy<typename T::InputType>& converter) {
        typename T::InputType;
        typename T::OutputType;
        std::derived_from<T, process::BaseTask<typename T::InputType, typename T::OutputType>>;
        { writer.run(converter, 0) } -> std::same_as<typename T::OutputType>;
        { writer.run(converter) } -> std::same_as<typename T::OutputType>;
        { writer() } -> std::same_as<typename T::OutputType>;
        { writer(0) } -> std::same_as<typename T::OutputType>;
        not std::copyable<T>;
        std::movable<T>;
    };
} // namespace srs::writer
