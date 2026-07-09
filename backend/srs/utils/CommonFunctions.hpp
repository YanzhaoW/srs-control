#pragma once

#include "EnumConvertFunctions.hpp"  // IWYU pragma: export
#include "srs/utils/CommonAlias.hpp" // IWYU pragma: keep
#include "srs/utils/CommonDefinitions.hpp"
#include <asio/any_io_executor.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/use_future.hpp>
#include <bit>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <magic_enum/magic_enum.hpp>
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

    template <std::size_t low_size, std::size_t total_size>
        requires(total_size <= sizeof(std::uint64_t) * common::BYTE_BIT_LENGTH and total_size >= low_size)
    constexpr auto get_low_bits(const std::bitset<total_size>& bits) -> std::bitset<low_size>
    {
        constexpr auto high_size = total_size - low_size;
        const auto low_bits = std::bitset<low_size>{ ((bits << high_size) >> high_size).to_ullong() };
        return low_bits;
    }

    template <std::size_t high_size, std::size_t total_size>
        requires(total_size <= sizeof(std::uint64_t) * common::BYTE_BIT_LENGTH and total_size >= high_size)
    constexpr auto get_high_bits(const std::bitset<total_size>& bits) -> std::bitset<high_size>
    {
        constexpr auto low_size = total_size - high_size;
        const auto high_bits = std::bitset<high_size>{ (bits >> low_size).to_ullong() };
        return high_bits;
    }

    template <std::size_t bit_size>
    constexpr auto byte_swap(const std::bitset<bit_size>& bits)
    {
        auto val = bits.to_ullong();
        val = val << ((sizeof(uint64_t) * common::BYTE_BIT_LENGTH) - bit_size);
        val = std::byteswap(val);
        return std::bitset<bit_size>(val);
    }

    template <typename T>
    constexpr auto gray_to_binary(T gray_val)
    {
        auto bin_val = T{ gray_val };
        while (gray_val > 0)
        {
            gray_val >>= 1U;
            bin_val ^= gray_val;
        }
        return bin_val;
    }

    template <typename T>
    constexpr auto binary_to_gray(T bin_val)
    {
        return bin_val ^ (bin_val >> 1U);
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

    inline auto get_default_log_path() -> std::filesystem::path
    {
        using path = std::filesystem::path;
        const auto* xdg_state_home = std::getenv("XDG_STATE_HOME");
        const auto state_dir =
            xdg_state_home == nullptr ? path{ std::getenv("HOME") } / path{ ".local/state" } : path{ xdg_state_home };
        return state_dir / path{ "srs_control/srs_control.log" };
    }
} // namespace srs::common
