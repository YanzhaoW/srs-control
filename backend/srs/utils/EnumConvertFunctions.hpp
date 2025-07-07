// IWYU pragma: private
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace srs::common::internal
{

    constexpr auto enum_to_dash_converter = [](const auto char_val)
    {
        if (char_val == '_')
        {
            return '-';
        }
        return char_val;
    };

    constexpr auto enum_from_dash_converter = [](const auto char_val)
    {
        if (char_val == '-')
        {
            return '_';
        }
        return char_val;
    };

    template <typename Enum>
    constexpr auto get_enum_dashed_names()
    {
        constexpr auto enum_names = magic_enum::enum_names<Enum>();
        constexpr auto enum_size = enum_names.size();
        constexpr auto MAX_SIZE = 20 * enum_size;
        constexpr auto arr = [&]()
        {
            auto output_str = std::array<char, MAX_SIZE>{};
            auto tmp_vec = enum_names | std::views::join | std::views::transform(enum_to_dash_converter) |
                           std::ranges::to<std::vector<char>>();
            std::ranges::copy(tmp_vec, output_str.begin());
            return std::make_pair(tmp_vec.size(), output_str);
        }();
        static constexpr auto output_arr = [&]()
        {
            auto output_str = std::array<char, arr.first>{};
            std::ranges::copy(arr.second | std::ranges::views::take(arr.first), output_str.begin());
            return output_str;
        }();
        constexpr auto output_str_vs = [&]()
        {
            auto arr = std::array<std::string_view, enum_size>{};
            auto front_iter = output_arr.begin();
            // TODO: use zip_view
            for (auto idx = std::size_t{}; idx < enum_size; ++idx)
            {
                const auto distance = enum_names[idx].size();
                arr[idx] = std::string_view{ front_iter, front_iter + distance };
                front_iter += distance;
            }
            return arr;
        }();
        return output_str_vs;
    }

    template <auto EnumVal>
    constexpr auto get_enum_dashed_name()
    {
        constexpr auto enum_name = magic_enum::enum_name<decltype(EnumVal)>(EnumVal);
        static constexpr auto enum_arr = [&]()
        {
            auto arr = std::array<char, enum_name.size()>{};
            auto tmp_vec =
                enum_name | std::views::transform(enum_to_dash_converter) | std::ranges::to<std::vector<char>>();
            std::ranges::copy(tmp_vec, arr.begin());
            return arr;
        }();
        return std::string_view{ enum_arr.begin(), enum_arr.end() };
    }

    constexpr auto get_enum_dashed_name(auto EnumVal) -> std::string
    {
        return magic_enum::enum_name<decltype(EnumVal)>(EnumVal) | std::views::transform(enum_to_dash_converter) |
               std::ranges::to<std::string>();
    }

    template <typename Enum>
    constexpr auto get_enum(std::string_view enum_name)
    {
        return magic_enum::enum_cast<Enum>(
            enum_name | std::views::transform(enum_from_dash_converter) | std::ranges::to<std::string>(),
            magic_enum::case_insensitive);
    }

    template <typename Enum>
    constexpr auto get_enum_dash_map()
    {
        return magic_enum::enum_values<Enum>() |
               std::views::transform([](auto enum_val)
                                     { return std::make_pair(get_enum_dashed_name(enum_val), enum_val); }) |
               std::ranges::to<std::map<std::string, Enum>>();
    }
} // namespace srs::common::internal
