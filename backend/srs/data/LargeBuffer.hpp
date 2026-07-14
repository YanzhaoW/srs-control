#pragma once

#include "srs/utils/CommonAlias.hpp"
#include <cstddef>
#include <fmt/base.h>
#include <span>
#include <string_view>

namespace srs
{
    /**
     * @brief Class for buffer for storing large data.
     *
     * It has two different length indicators: size and buffer size (capacity). Similar to `std::vector`, size
     * represents the current amount of valid values and buffer size represents the memory allocated for the buffer. The
     * reason to use LargeBuffer instead of simply `std::vector` is because the
     * [std::vector::resize()](https://en.cppreference.com/cpp/container/vector/resize) also initialize the values when
     * vector grows, while `LargeBuffer::resize()` just change the size value, without furthering operations. The
     * LargeBuffer is not supposed to grow or reallocated and all the memory should be allocated up front via the
     * constructor `LargeBuffer(std::size_t)`.
     */
    class LargeBuffer
    {
      public:
        /**
         * @brief Default constructor for an empty buffer. This does not allocate any memory.
         *
         */
        LargeBuffer() = default;

        /**
         * @brief Constructor with a buffer size.
         *
         * The number of the elements pre-allocated is equal to the buffer_size.
         *
         * @param buffer_size Number of element whose memory pre-allocated to the buffer.
         */
        explicit LargeBuffer(std::size_t buffer_size) { data_.resize(buffer_size); }

        /**
         * @brief Default copy constructor. Very slow and should be avoided.
         */
        explicit LargeBuffer(const LargeBuffer& other) = delete;

        /**
         * @brief Move constructor via swapping.
         *
         * This "move" constructor does not move the data of the other, but rather perform the swap action with
         * other object.
         * @param other Other LargeBuffer object to be swapped.
         */
        explicit LargeBuffer(LargeBuffer&& other) noexcept = default;

        /**
         * @brief Default copy assignment. Very slow and should be avoided.
         */
        auto operator=(const LargeBuffer& other) -> LargeBuffer& = delete;

        /**
         * @brief Move assignment via swapping.
         *
         * This "move" assignment does not move the data of the other, but rather perform the swap action with
         * other object.
         * @param other Other LargeBuffer object to be swapped.
         */
        auto operator=(LargeBuffer&& other) noexcept -> LargeBuffer& = default;

        /**
         * @brief Default destructor.
         */
        ~LargeBuffer() = default;

        /**
         * @brief Comparison with a span.
         *
         * @param msg Span data.
         * @return True if equal
         */
        auto operator==(std::span<char> msg) const -> bool { return std::string_view{ msg } == data(); }

        /**
         * @brief Comparison with a string_view.
         *
         * @param msg string_view data.
         * @return True if equal
         */
        auto operator==(std::string_view msg) const -> bool { return msg == data(); }

        /**
         * @brief Return a string_view with the underlying data and current size.
         *
         * @return string_view data
         */
        [[nodiscard]] auto data() const -> std::string_view { return std::string_view{ data_.data(), size_ }; }
        [[nodiscard]] auto empty() const -> bool { return data_.empty(); }

        /**
         * @brief Change the size of this buffer.
         *
         * Changing the size to the new size value, without assigning or changing the old data values.
         * @param size New size value.
         */
        void resize(std::size_t size)
        {
            size_ = size;
            data_.resize(size_ >= data_.size() ? size_ : data_.size());
        }
        // void set_buffer_size(std::size_t capacity) { data_.resize(capacity); }

        /**
         * @brief Getter for the buffer size.
         *
         * @return Buffer size
         */
        [[nodiscard]] auto get_buffer_size() const -> std::size_t { return data_.size(); }

        /**
         * @brief Getter for the size.
         *
         * @return Size of elements in the buffer
         */
        [[nodiscard]] auto get_size() const -> std::size_t { return size_; }

        /**
         * @brief Return the total underlying data.
         *
         * @return Span of the underlying data.
         */
        // INFO: Do not return the reference to the underlying vector, as it's not thread safe.
        auto get_all_data() -> std::span<char> { return std::span{ data_ }; }

        auto is_empty() -> bool { return data_.size() == 0 and data_.capacity() == 0; }

      private:
        std::size_t size_ = 0;
        BinaryData data_;
    };

} // namespace srs
