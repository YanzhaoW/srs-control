#pragma once

#ifdef HAS_ROOT
#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>

#include <srs/data/SRSDataStructs.hpp>
#include <srs/workflow/TaskDiagram.hpp>

namespace srs::writer
{
    class RootFile
    {
      public:
        using InputType = const StructData*;
        using OutputType = int;
        using CoroType = asio::experimental::coro<OutputType(std::optional<InputType>)>;
        using InputFuture = boost::shared_future<std::optional<InputType>>;
        using OutputFuture = boost::unique_future<std::optional<OutputType>>;
        static constexpr auto IsStructType = true;

        explicit RootFile(asio::thread_pool& thread_pool, auto&&... args)
            : root_file{ std::forward<decltype(args)>(args)... }
        {
            spdlog::debug("Root file {:?} has been opened.", root_file.GetName());
            tree.SetDirectory(&root_file);
            tree.Branch("srs_frame_data", &output_buffer_);
            coro_ = generate_coro(thread_pool.get_executor());
            common::coro_sync_start(coro_, std::optional<InputType>{}, asio::use_awaitable);
            // spdlog::trace("ROOT INCLUDE DIRS: {}", gSystem->GetIncludePath());
            // spdlog::trace("ROOT LIBRARIES: {}", gSystem->GetLibraries());
        }

        ~RootFile()
        {
            if (not is_closed_)
            {
                spdlog::error("Root file {:?} is not closed successfully. Data might not be written completely.",
                              root_file.GetName());
            }
        }

        auto write(auto pre_future) -> OutputFuture { return common::create_coro_future(coro_, pre_future); }
        [[nodiscard]] static auto get_convert_mode() -> process::DataConvertOptions
        {
            return process::DataConvertOptions::structure;
        }

      private:
        bool is_closed_ = false;
        TFile root_file;
        CoroType coro_;
        TTree tree{ "srs_data_tree", "Data structures from SRS system" };
        StructData output_buffer_;

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
                    output_buffer_ = *data_struct;
                    tree.Fill();
                    res = 1;
                }
                auto data_temp = co_yield res;
                if (data_temp.has_value())
                {
                    data_struct = data_temp.value();
                }
                else
                {
                    root_file.Write();
                    spdlog::info("ROOT file {} is closed successfully", root_file.GetName());
                    is_closed_ = true;
                    co_return;
                }
            }
        }

      public:
        RootFile(const RootFile&) = delete;
        RootFile(RootFile&&) = delete;
        RootFile& operator=(const RootFile&) = delete;
        RootFile& operator=(RootFile&&) = delete;
    };

} // namespace srs::writer

#endif
