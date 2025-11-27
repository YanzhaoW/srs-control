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
        };

    } // namespace internal

    template <typename T>
    concept WritableFile = requires(T writer) {
        typename T::InputType;
        typename T::OutputType;
        std::derived_from<T, process::BaseTask<typename T::InputType, typename T::OutputType>>;
        writer.run_task();
    };
} // namespace srs::writer
