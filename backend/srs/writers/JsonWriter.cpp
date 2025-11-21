#include "JsonWriter.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glaze/core/opts.hpp>
#include <glaze/core/write.hpp>
#include <ios>
#include <ranges>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace srs::writer
{
    void CompactExportData::set_value(const StructData& data_struct)
    {
        header = data_struct.header;
        fill_hit_data(data_struct.hit_data);
        fill_marker_data(data_struct.marker_data);
    }

    void CompactExportData::fill_hit_data(const std::vector<HitData>& hits)
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

    void CompactExportData::fill_marker_data(const std::vector<MarkerData>& markers)
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

    Json::Json(const std::string& filename, std::size_t n_lines)
        : WriterTask{ "JSONWriter", structure, n_lines }
        , filename_{ filename }
    {
        assert(n_lines > 0);
        is_first_item_.resize(n_lines);
        output_data_.resize(n_lines);
        file_streams_.reserve(n_lines);
        data_buffers_.reserve(n_lines);
        string_buffers_.reserve(n_lines);

        for (auto idx : std::views::iota(0, static_cast<int>(n_lines)))
        {
            auto full_filename = (n_lines == 1) ? filename : common::insert_index_to_filename(filename, idx);
            file_streams_.emplace_back(full_filename, std::ios::out | std::ios::trunc);
        }

        for (auto& file_stream : file_streams_)
        {
            if (not file_stream.is_open())
            {
                spdlog::critical("JsonWriter: cannot open the file with filename {:?}", filename);
                throw std::runtime_error("Error occurred with JsonWriter");
            }
            file_stream << "[\n";
        }
    }
    Json::~Json()
    {
        for (auto [idx, file_stream] : std::views::enumerate(file_streams_))
        {
            file_stream << "]\n";
            file_stream.close();
            spdlog::info("JSON file {} with index {} is closed successfully", filename_, idx);
        }
    }

    void Json::write_json(const StructData& data_struct, std::size_t line_num)
    {
        if (not is_first_item_[line_num].value)
        {
            file_streams_[line_num] << ", ";
        }
        else
        {
            is_first_item_[line_num].value = false;
        }

        data_buffers_[line_num].set_value(data_struct);
        auto error_code = glz::write<glz::opts{ .prettify = true, .new_lines_in_arrays = false }>(
            data_buffers_[line_num], string_buffers_[line_num]);
        if (error_code)
        {
            spdlog::critical("JsonWriter: cannot interpret data struct to json. Error: {}",
                             error_code.custom_error_message);
            throw std::runtime_error("Error occurred with JsonWriter");
        }
        output_data_[line_num] = string_buffers_[line_num].size();
        file_streams_[line_num] << string_buffers_[line_num];
        string_buffers_[line_num].clear();
    }
} // namespace srs::writer
