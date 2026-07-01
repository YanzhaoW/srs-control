#pragma once

#include "srs/utils/CommonAlias.hpp"
#include <cstddef>
#include <fmt/base.h>
#include <span>
#include <string_view>

namespace srs
{
    class LargeBuffer
    {
      public:
        LargeBuffer() = default;
        LargeBuffer(std::size_t capacity) { data_.resize(capacity); }

        explicit LargeBuffer(LargeBuffer& other)
        {
            data_.swap(other.data_);
            size_ = other.size_;
        }
        explicit LargeBuffer(const LargeBuffer& other) = default;
        explicit LargeBuffer(LargeBuffer&& other) = default;
        auto operator=(const LargeBuffer& other) -> LargeBuffer& = default;
        auto operator=(LargeBuffer&& other) -> LargeBuffer& = default;

        ~LargeBuffer() = default;

        auto operator==(std::span<char> msg) const -> bool { return std::string_view{ msg } == data(); }

        auto operator==(std::string_view msg) const -> bool { return msg == data(); }

        [[nodiscard]] auto data() const -> std::string_view { return std::string_view{ data_.data(), size_ }; }
        [[nodiscard]] auto empty() const -> bool { return data_.empty(); }

        void clear() { data_.clear(); }

        void resize(std::size_t size)
        {
            size_ = size;
            data_.resize(size_ >= data_.size() ? size_ : data_.size());
        }
        // void set_buffer_size(std::size_t capacity) { data_.resize(capacity); }

        [[nodiscard]] auto get_buffer_size() const -> auto { return data_.size(); }

        [[nodiscard]] auto get_size() const -> auto { return size_; }

        auto get_all_data() -> std::span<char> { return std::span{ data_ }; }

      private:
        std::size_t size_ = 0;
        BinaryData data_;
    };

} // namespace srs
