#pragma once

#include <fstream>
#include <map>

#include <glaze/glaze.hpp>

#include <srs/workflow/TaskDiagram.hpp>

namespace srs::writer
{
    struct CompactExportData
    {
        ReceiveDataHeader header{};
        std::size_t marker_size{};
        std::size_t hit_size{};
        std ::map<std::string, std::vector<uint64_t>> marker_data{ { "vmm_id", {} }, { "srs_timestamp", {} } };
        std ::map<std::string, std::vector<uint16_t>> hit_data{ { "is_over_threshold", {} },
                                                                { "channel_num", {} },
                                                                { "tdc", {} },
                                                                { "offset", {} },
                                                                { "vmm_id", {} },
                                                                { "adc", {} },
                                                                { "bc_id", {} } };
        // HitDataVec hit_data;

        void set_value(const StructData& data_struct)
        {
            header = data_struct.header;
            fill_hit_data(data_struct.hit_data);
            fill_marker_data(data_struct.marker_data);
        }

      private:
        void fill_hit_data(const std::vector<HitData>& hits)
        {
            hit_size = hits.size();
            for (auto& [key, value] : hit_data)
            {
                value.clear();
                value.reserve(hit_size);
            }
            for (const auto& hit : hits)
            {
                hit_data.at("is_over_threshold").push_back(static_cast<uint16_t>(hit.is_over_threshold));
                hit_data.at("channel_num").push_back(hit.channel_num);
                hit_data.at("tdc").push_back(hit.tdc);
                hit_data.at("offset").push_back(hit.offset);
                hit_data.at("vmm_id").push_back(hit.vmm_id);
                hit_data.at("adc").push_back(hit.adc);
                hit_data.at("bc_id").push_back(hit.bc_id);
            }
        }

        void fill_marker_data(const std::vector<MarkerData>& markers)
        {
            marker_size = markers.size();
            for (auto& [key, value] : marker_data)
            {
                value.clear();
                value.reserve(marker_size);
            }
            for (const auto& marker : markers)
            {
                marker_data.at("vmm_id").push_back(marker.vmm_id);
                marker_data.at("srs_timestamp").push_back(marker.srs_timestamp);
            }
        }
    };

    class Json
    {
      public:
        using InputType = const StructData*;
        using OutputType = int;
        using CoroType = asio::experimental::coro<OutputType(std::optional<InputType>)>;
        using InputFuture = boost::shared_future<std::optional<InputType>>;
        using OutputFuture = boost::unique_future<std::optional<OutputType>>;
        static constexpr auto IsStructType = true;

        explicit Json(asio::thread_pool& thread_pool, const std::string& filename)
            : filename_{ filename }
            , file_stream_{ filename, std::ios::out | std::ios::trunc }
        {
            if (not file_stream_.is_open())
            {
                spdlog::critical("JsonWriter: cannot open the file with filename {:?}", filename);
                throw std::runtime_error("Error occured with JsonWriter");
            }
            file_stream_ << "[\n";
            coro_ = generate_coro(thread_pool.get_executor());
            common::coro_sync_start(coro_, std::optional<InputType>{}, asio::use_awaitable);
        }

        [[nodiscard]] static auto get_convert_mode() -> process::DataConvertOptions
        {
            return process::DataConvertOptions::structure;
        }
        auto write(auto pre_future) -> boost::unique_future<std::optional<int>>
        {
            return common::create_coro_future(coro_, pre_future);
        }

      private:
        bool is_first_item = true;
        std::string filename_;
        std::fstream file_stream_;
        CompactExportData data_buffer_;
        std::string string_buffer_;
        CoroType coro_;

        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        auto generate_coro(asio::any_io_executor /*unused*/) -> CoroType
        {
            InputType data_struct{};
            auto res = 0;
            while (true)
            {
                res = 0;
                if (data_struct != nullptr)
                {
                    write_json(*data_struct);
                    res = 1;
                }
                auto data_temp = co_yield res;
                if (data_temp.has_value())
                {
                    data_struct = data_temp.value();
                }
                else
                {
                    file_stream_ << "]\n";
                    file_stream_.close();
                    spdlog::info("JSON file {} is closed successfully", filename_);
                    co_return;
                }
            }
        }

        void write_json(const StructData& data_struct)
        {
            if (not is_first_item)
            {
                file_stream_ << ", ";
            }
            else
            {
                is_first_item = false;
            }

            data_buffer_.set_value(data_struct);
            auto error_code =
                glz::write<glz::opts{ .prettify = 1, .new_lines_in_arrays = 0 }>(data_buffer_, string_buffer_);
            if (error_code)
            {
                spdlog::critical("JsonWriter: cannot interpret data struct to json. Error: {}",
                                 error_code.custom_error_message);
                throw std::runtime_error("Error occured with JsonWriter");
            }
            file_stream_ << string_buffer_;
            string_buffer_.clear();
        }
    };

} // namespace srs::writer
