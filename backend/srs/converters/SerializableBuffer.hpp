#pragma once

#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <srs/utils/CommonAlias.hpp>
#include <zpp_bits.h>

namespace srs::process
{
    class SerializableMsgBuffer
    {
      public:
        SerializableMsgBuffer() = default;
        explicit SerializableMsgBuffer(std::span<BufferElementType> read_data)
        {
            data_.reserve(read_data.size());
            std::ranges::copy(read_data, std::back_inserter(data_));
        }

        auto serialize(auto&&... structs)
        {
            auto serialize_to = zpp::bits::out{ data_, zpp::bits::endian::network{}, zpp::bits::no_size{} };
            auto size = data_.size();
            serialize_to.position() += sizeof(BufferElementType) * size;

            // cista::buf<WriteBufferType&> serializer{ buffer };
            // serialize_multi(serialize_to, std::forward<decltype(structs)>(structs)...);
            serialize_to(std::forward<decltype(structs)>(structs)...).or_throw();
            return asio::buffer(data_);
        }

        [[nodiscard]] auto data() const -> std::string_view { return std::string_view{ data_.data(), data_.size() }; }

        void clear() { data_.clear(); }

      private:
        BinaryData data_;
    };
} // namespace srs
