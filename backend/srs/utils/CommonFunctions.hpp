#pragma once

#include <bit>
#include <bitset>
#include <boost/asio.hpp>
#include <boost/thread/future.hpp>
#include <srs/utils/CommonAlias.hpp>

namespace srs::common
{
    /**  \defgroup CommonFunctions Common functions
     *   @{
     */

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

    auto create_coro_future(auto& coro, auto&& pre_fut)
    {
        if (not pre_fut.valid())
        {
            throw std::runtime_error("Previous future is not valid!");
        }
        return pre_fut.then(
            [&coro](std::remove_cvref_t<decltype(pre_fut)> fut)
            {
                ;
                return asio::co_spawn(
                           coro.get_executor(), coro.async_resume(fut.get(), asio::use_awaitable), asio::use_future)
                    .get();
            });
    }

    auto create_coro_future(auto& coro, bool is_terminated)
    {
        return boost::async(
            [&coro, is_terminated]()
            {
                return asio::co_spawn(
                           coro.get_executor(), coro.async_resume(is_terminated, asio::use_awaitable), asio::use_future)
                    .get();
            });
    }

    void coro_sync_start(auto& coro, auto&&... args)
    {
        asio::co_spawn(coro.get_executor(), coro.async_resume(std::forward<decltype(args)>(args)...), asio::use_future)
            .get();
    }

    /** @}*/
} // namespace srs
