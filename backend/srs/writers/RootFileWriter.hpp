#pragma once

#include "DataWriterOptions.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include <cassert>
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#ifdef HAS_ROOT
#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>

namespace srs::writer
{
    class RootFile : public process::WriterTask<DataWriterOption::root, const StructData*, std::size_t>
    {
      public:
        static constexpr auto IsStructType = true;

        RootFile(const std::string& filename, process::DataConvertOptions convert_mode, std::size_t n_lines);

        void run_task(const auto& prev_data_converter, std::size_t line_number)
        {
            assert(line_number < get_n_lines());
            const auto* input_data = prev_data_converter.get_data_view(line_number);
            assert(input_data != nullptr);
            data_struct_buffers_[line_number] = *input_data;
            output_data_[line_number] = static_cast<std::size_t>(trees_[line_number]->Fill());
        }

        ~RootFile();
        RootFile(const RootFile&) = delete;
        RootFile(RootFile&&) = delete;
        RootFile& operator=(const RootFile&) = delete;
        RootFile& operator=(RootFile&&) = delete;

      private:
        std::string base_filename_;
        std::vector<std::unique_ptr<TFile>> root_files_;
        std::vector<TTree*> trees_;
        std::vector<StructData> data_struct_buffers_;
        std::vector<OutputType> output_data_;
    };

} // namespace srs::writer

#endif
