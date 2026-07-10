#pragma once

#include <glaze/core/meta.hpp>
#include <magic_enum/magic_enum.hpp>

namespace glz
{
    template <typename T>
        requires std::is_enum_v<T>
    struct meta<T>
    {
        static constexpr auto keys = magic_enum::enum_names<T>();
        static constexpr auto value = magic_enum::enum_values<T>();
    };
} // namespace glz
