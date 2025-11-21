#pragma once

#include "srs/writers/BinaryFileWriter.hpp"
#include "srs/writers/JsonWriter.hpp"
#ifdef HAS_ROOT
#include "srs/writers/RootFileWriter.hpp"
#endif
#include "srs/writers/UDPWriter.hpp"
#include <boost/thread/future.hpp>
#include <concepts>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>

#include <srs/converters/DataConvertOptions.hpp>
#include <srs/data/SRSDataStructs.hpp>
#include <srs/writers/DataWriterOptions.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace srs::workflow
{
    class Handler;
} // namespace srs::workflow

namespace srs::writer
{
    template <typename T>
    concept WriterVisitor = requires(T visitor, BinaryFile& binary_file, BinaryFile& UDP_file, BinaryFile& JSON_file) {
        { visitor(std::string_view{}, binary_file) } -> std::same_as<void>;
#ifdef HAS_ROOT
        { T(std::string_view{}, std::declval<RootFile>()) } -> std::same_as<void>;
#endif
        { visitor(std::string_view{}, UDP_file) } -> std::same_as<void>;
        { visitor(std::string_view{}, JSON_file) } -> std::same_as<void>;
    };

    class Manager
    {
      public:
        using enum DataWriterOption;
        using enum process::DataConvertOptions;

        explicit Manager(workflow::Handler* processor);

        ~Manager();
        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = default;
        Manager& operator=(Manager&&) = default;

        void write_with(auto make_future);
        void wait_for_finished();
        void reset() { write_futures_.clear(); }
        void set_output_filenames(const std::vector<std::string>& filenames);
        [[nodiscard]] auto get_binary_writers() const -> const auto& { return binary_files_; }
        void do_for_each_writer(WriterVisitor auto visitor);
        void do_for_each_writer(WriterVisitor auto visitor) const;

        // Getter:
        [[nodiscard]] auto is_convert_required(process::DataConvertOptions dependee) const -> bool;
        [[nodiscard]] auto generate_conversion_req_map() const -> std::map<process::DataConvertOptions, bool>;

      private:
        // std::map<process::DataConvertOptions, std::size_t> convert_count_map_{
        //     process::EMPTY_CONVERT_OPTION_COUNT_MAP.begin(),
        //     process::EMPTY_CONVERT_OPTION_COUNT_MAP.end()
        // };
        std::map<std::string, std::unique_ptr<BinaryFile>> binary_files_;
        std::map<std::string, std::unique_ptr<UDP>> udp_files_;
        std::map<std::string, std::unique_ptr<Json>> json_files_;
#ifdef HAS_ROOT
        std::map<std::string, std::unique_ptr<RootFile>> root_files_;
#endif
        workflow::Handler* workflow_handler_ = nullptr;
        std::vector<boost::unique_future<std::optional<std::size_t>>> write_futures_;

        auto add_binary_file(const std::string& filename, process::DataConvertOptions deser_mode) -> bool;
        auto add_udp_file(const std::string& filename, process::DataConvertOptions deser_mode) -> bool;
        auto add_root_file(const std::string& filename) -> bool;
        auto add_json_file(const std::string& filename) -> bool;

        template <typename WriterType>
        void write_to_files(std::map<std::string, std::unique_ptr<WriterType>>& writers, auto make_future)
        {
            for (auto& [key, writer] : writers)
            {
                auto fut = make_future(*writer);
                write_futures_.emplace_back(std::move(fut));
            }
        }

        template <typename WriterType>
        void for_each_file(std::map<std::string, std::unique_ptr<WriterType>>& writers, auto visitor)
        {
            for (auto& [key, writer] : writers)
            {
                visitor(key, *writer);
            }
        }
        template <typename WriterType>
        void for_each_file(const std::map<std::string, std::unique_ptr<WriterType>>& writers, auto visitor) const
        {
            for (const auto& [key, writer] : writers)
            {
                visitor(key, *writer);
            }
        }
    };

    void Manager::do_for_each_writer(WriterVisitor auto visitor)
    {
        for_each_file(binary_files_, visitor);
        for_each_file(udp_files_, visitor);
        for_each_file(json_files_, visitor);
#ifdef HAS_ROOT
        for_each_file(root_files_, visitor);
#endif
    }

    void Manager::do_for_each_writer(WriterVisitor auto visitor) const
    {
        for_each_file(binary_files_, visitor);
        for_each_file(udp_files_, visitor);
        for_each_file(json_files_, visitor);
#ifdef HAS_ROOT
        for_each_file(root_files_, visitor);
#endif
    }

    void Manager::write_with(auto make_future)
    {
        write_to_files(binary_files_, make_future);
        write_to_files(udp_files_, make_future);
        write_to_files(json_files_, make_future);
#ifdef HAS_ROOT
        write_to_files(root_files_, make_future);
#endif
    }
} // namespace srs::writer
