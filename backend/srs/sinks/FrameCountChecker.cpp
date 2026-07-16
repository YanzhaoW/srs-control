#include "FrameCountChecker.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/workflow/BaseTask.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <fmt/base.h>
#include <zpp_bits.h>

namespace srs::sink
{
    FrameCountChecker::FrameCountChecker(std::size_t n_lines)
        : SinkTask{ "FrameCountChecker", process::DataConvertOptions::raw, n_lines }
    {
    }

    auto FrameCountChecker::extract_frame_counter(InputType binary_data) -> RunResult
    {

        auto deserialize_to = zpp::bits::in{ binary_data, zpp::bits::endian::network{}, zpp::bits::no_size{} };
        auto read_bytes = binary_data.size() * sizeof(BufferElementType);
        auto header = ReceiveDataHeader{};
        constexpr auto header_bytes = sizeof(header);
        if (read_bytes <= header_bytes)
        {
            return std::unexpected{ "Deserialization: The size of the binary data is too small!" };
        }
        auto err = deserialize_to(header);

        if (zpp::bits::failure(err))
        {
            return std::unexpected{ "FrameCountChecker failed to deserialize the header" };
        }
        return header.frame_counter;
    }

    void FrameCountChecker::register_frame_counter(int64_t frame_counter)
    {
        auto last_value = last_frame_counter_.load();
        while (true)
        {
            auto diff = frame_counter - last_value;
            auto insert_value = (diff > 0) ? frame_counter : last_value;
            if (last_frame_counter_.compare_exchange_weak(last_value, insert_value))
            {
                if (diff > 1)
                {
                    frame_drop_counter_ += (diff - 1);
                }
                else if (diff < 0)
                {
                    --frame_drop_counter_;
                }
                break;
            }
        }
    }

    auto FrameCountChecker::analyze_frame_counter(InputType binary_data) -> RunResult
    {
        auto frame_counter = extract_frame_counter(binary_data);

        if (frame_counter)
        {
            register_frame_counter(frame_counter.value());
        }

        return frame_counter;
    }

    FrameCountChecker::~FrameCountChecker() {}
} // namespace srs::sink
