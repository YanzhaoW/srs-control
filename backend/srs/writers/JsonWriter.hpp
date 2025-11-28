#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/writers/DataWriterOptions.hpp"
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/thread/future.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <glaze/core/opts.hpp>
#include <glaze/core/write.hpp>
#include <glaze/glaze.hpp>
#include <map>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

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

        void set_value(const StructData& data_struct);

      private:
        void fill_hit_data(const std::vector<HitData>& hits);
        void fill_marker_data(const std::vector<MarkerData>& markers);
    };

    class Json : public process::WriterTask<DataWriterOption::json, const StructData*, std::size_t>
    {
      public:
        static constexpr auto IsStructType = true;
        struct Bool
        {
            bool value = true;
            explicit operator bool() const { return value; }
        };

        explicit Json(const std::string& filename, process::DataConvertOptions convert_mode, std::size_t n_lines = 1);

        Json(const Json&) = delete;
        Json(Json&&) = default;
        Json& operator=(const Json&) = delete;
        Json& operator=(Json&&) = default;
        ~Json();

        [[nodiscard]] auto operator()(std::size_t line_number = 0) const -> OutputType
        {
            return output_data_[line_number];
        }

        [[nodiscard]] auto get_filename() const -> const std::string& { return filename_; }

        auto run(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number = 0) -> OutputType
        {
            assert(line_number < get_n_lines());
            const auto* data_struct = prev_data_converter(line_number);
            write_json(*data_struct, line_number);
            return this->operator()(line_number);
        }

      private:
        static constexpr auto name_ = std::string_view{};
        std::vector<Bool> is_first_item_;
        std::string filename_;
        std::vector<OutputType> output_data_;
        std::vector<std::fstream> file_streams_;
        std::vector<CompactExportData> data_buffers_;
        std::vector<std::string> string_buffers_;

        void write_json(const StructData& data_struct, std::size_t line_num);
    };

} // namespace srs::writer
