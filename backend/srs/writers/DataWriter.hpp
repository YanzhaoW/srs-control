#pragma once

#include "DataStructs.hpp"
#include "DataWriterOptions.hpp"
#include <boost/thread/future.hpp>
#include <map>
// #include <srs/analysis/DataProcessManager.hpp>
#include <srs/serializers/DataDeserializeOptions.hpp>

namespace srs
{
    class RootFileWriter;
    class DataProcessor;
    class BinaryFileWriter;
    class UDPWriter;
    class JsonWriter;
    class DataProcessManager;

    class DataWriter
    {
      public:
        using enum DataWriterOption;
        using enum DataDeserializeOptions;

        explicit DataWriter(DataProcessor* processor);

        ~DataWriter();
        DataWriter(const DataWriter&) = delete;
        DataWriter& operator=(const DataWriter&) = delete;
        DataWriter(DataWriter&&) = default;
        DataWriter& operator=(DataWriter&&) = default;

        void write_with(auto make_future);
        void wait_for_finished();
        void reset() { write_futures_.clear(); }

        // setters:
        void set_output_filenames(const std::vector<std::string>& filenames);

        // Getter:
        [[nodiscard]] auto has_deserialize_type(DataDeserializeOptions option) const -> bool;

      private:
        std::map<DataDeserializeOptions, int> output_num_trackers_{ { raw, 0 },
                                                                    { structure, 0 },
                                                                    { proto, 0 },
                                                                    { proto_frame, 0 } };
        std::map<std::string, std::unique_ptr<BinaryFileWriter>> binary_files_;
        std::map<std::string, std::unique_ptr<UDPWriter>> udp_files_;
        std::map<std::string, std::unique_ptr<JsonWriter>> json_files_;
#ifdef HAS_ROOT
        std::map<std::string, std::unique_ptr<RootFileWriter>> root_files_;
#endif
        DataProcessor* data_processor_ = nullptr;
        std::vector<boost::unique_future<std::optional<int>>> write_futures_;

        void write_struct_json(const StructData& data_struct);
        void write_struct_root(const StructData& data_struct);

        void write_binary_file(const BinaryData& read_data);
        static void write_binary_udp(const BinaryData& read_data);
        [[nodiscard]] auto check_if_exist(DataDeserializeOptions option) const -> bool
        {
            return output_num_trackers_.at(option) > 0;
        }

        template <typename WriterType>
        void write_to_files(std::map<std::string, std::unique_ptr<WriterType>>& writers, auto make_future)
        {
            for (auto& [key, writer] : writers)
            {
                auto fut = make_future(*writer);
                write_futures_.emplace_back(std::move(fut));
            }
        }
    };

    void DataWriter::write_with(auto make_future)
    {
        write_to_files(binary_files_, make_future);
        write_to_files(udp_files_, make_future);
        write_to_files(json_files_, make_future);
#ifdef HAS_ROOT
        write_to_files(root_files_, make_future);
#endif
    }
} // namespace srs
