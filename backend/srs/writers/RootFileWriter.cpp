#ifdef HAS_ROOT
#include "RootFileWriter.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>
#include <cstddef>
#include <memory>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>

namespace srs::writer
{

    RootFile::RootFile(const std::string& filename, process::DataConvertOptions convert_mode, std::size_t n_lines)
        : WriterTask{ "RootFile", convert_mode, n_lines }
        , base_filename_{ filename }
    {
        root_files_.resize(n_lines);
        trees_.resize(n_lines);
        output_data_.resize(n_lines);
        data_struct_buffers_.resize(n_lines);
        for (auto [idx, root_file, tree] : std::views::zip(std::views::iota(0), root_files_, trees_))
        {
            auto full_filename = (n_lines == 1) ? filename : common::insert_index_to_filename(filename, idx);
            root_file = std::make_unique<TFile>(full_filename.c_str(), "RECREATE");
            // NOTE: tree is owned by the TFile
            tree = std::make_unique<TTree>("srs_data_tree", "Data structures from SRS system").release();
            tree->SetDirectory(root_file.get());
            tree->Branch("srs_frame_data", &data_struct_buffers_[static_cast<std::size_t>(idx)]);
        }
    }

    RootFile::~RootFile()
    {
        for (auto [root_file, tree] : std::views::zip(root_files_, trees_))
        {
            root_file->WriteObject(tree, tree->GetName());
            root_file->Close();
            spdlog::info("Writer: Root file {:?} is Closed.", root_file->GetName());
        }
    }
} // namespace srs::writer
#endif
