#pragma once

#include "EnumConvertFunctions.hpp"  // IWYU pragma: export
#include "srs/utils/CommonAlias.hpp" // IWYU pragma: keep
#include "srs/utils/CommonConcepts.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <bit>
#include <bitset>
#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/thread/future.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace srs::common
{
    // subbits from a half open range [min, max)
    template <std::size_t bit_size, std::size_t max, std::size_t min = 0>
    constexpr auto subset(const std::bitset<bit_size>& bits) -> std::bitset<max - min>
    {
        constexpr auto max_size = 64;
        static_assert(max > min);
        static_assert(max_size >= (max - min));
        constexpr auto ignore_high = bit_size - max;

        auto new_bits = (bits << ignore_high) >> (ignore_high + min);
        return std::bitset<max - min>{ new_bits.to_ullong() };
    }

    template <std::size_t high_size, std::size_t low_size>
    constexpr auto merge_bits(const std::bitset<high_size>& high_bits, const std::bitset<low_size>& low_bits)
        -> std::bitset<high_size + low_size>
    {
        using NewBit = std::bitset<high_size + low_size>;
        constexpr auto max_size = 64;
        static_assert(max_size >= high_size + low_size);

        auto high_bits_part = NewBit(high_bits.to_ullong());
        auto low_bits_part = NewBit(low_bits.to_ullong());
        auto new_bits = (high_bits_part << low_size) | low_bits_part;
        return std::bitset<high_size + low_size>(new_bits.to_ullong());
    }

    template <std::size_t bit_size>
    constexpr auto byte_swap(const std::bitset<bit_size>& bits)
    {
        auto val = bits.to_ullong();
        val = val << (sizeof(uint64_t) * common::BYTE_BIT_LENGTH - bit_size);
        val = std::byteswap(val);
        return std::bitset<bit_size>(val);
    }

    template <typename T>
    constexpr auto gray_to_binary(T gray_val)
    {
        auto bin_val = T{ gray_val };
        while (gray_val > 0)
        {
            gray_val >>= 1;
            bin_val ^= gray_val;
        }
        return bin_val;
    }

    constexpr auto get_shared_from_this(auto&& obj)
    {
        return std::static_pointer_cast<std::remove_cvref_t<decltype(obj)>>(obj.shared_from_this());
    }

    template <typename Enum>
    consteval auto get_enum_names()
    {
        auto names = magic_enum::enum_names<srs::common::ActionMode>();
        return names;
    }

    constexpr auto insert_index_to_filename(std::string_view native_name, int idx) -> std::string
    {
        auto filepath = std::filesystem::path{ native_name };
        auto extension = filepath.extension().string();
        auto file_basename = filepath.replace_extension().string();
        return std::format("{}_{}{}", file_basename, idx, extension);
    }

    auto create_coro_task(auto task, asio::any_io_executor executor)
    {
        auto task_handle = task();
        using input_type = decltype(task_handle)::input_type;
        asio::co_spawn(executor, task_handle.async_resume(input_type{}, asio::use_awaitable), asio::use_future).get();
        return task_handle;
    }

} // namespace srs::common
