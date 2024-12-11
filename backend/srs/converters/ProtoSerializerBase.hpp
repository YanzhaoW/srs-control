#pragma once

#include <srs/data/message.pb.h>
#include <srs/converters/StructDeserializer.hpp>
#include <utility>

namespace srs::process
{
    template <typename Converter>
    class ProtoSerializerBase : public DataConverterBase<const proto::Data*, std::string_view>
    {
      public:
        explicit ProtoSerializerBase(asio::thread_pool& thread_pool, std::string name, Converter converter)
            : DataConverterBase{ generate_coro(thread_pool.get_executor()) }
            , name_{ std::move(name) }
            , converter_{ converter }
        {
        }
        [[nodiscard]] auto data() const -> std::string_view { return output_data_; }

      private:
        std::string name_;
        std::string output_data_;
        Converter converter_;

        void reset() { output_data_.clear(); }

        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        auto generate_coro(asio::any_io_executor /*unused*/) -> CoroType
        {
            InputType temp_data{};
            while (true)
            {
                if (temp_data != nullptr)
                {
                    reset();
                    converter_(*temp_data, output_data_);
                }
                auto data = co_yield (std::string_view{ output_data_ });
                if (data.has_value())
                {
                    temp_data = data.value();
                }
                else
                {
                    spdlog::debug("Shutting down {:?} serializer.", name_);
                    co_return;
                }
            }
        }
    };

} // namespace srs
