#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace srs
{
    class AppReport
    {
      public:
        struct TaskStat
        {
            double total_time_ms{};
            uint64_t total_sample_size{};
        };

        struct FecSwitchStat
        {
            using US = std::chrono::microseconds;
            using USCount = decltype(US{}.count());
            USCount send_time{};
            USCount receiv_time{};
            USCount send_time_offset{};
            std::string_view connection_name;
        };

        struct FrameReadingStat
        {
            std::size_t n_frames{};
            std::size_t total_time_ns{};
            std::size_t total_bytes_read{};
        };

        struct QueueStat
        {
            std::size_t empty_trash{};
            std::size_t full_trash{};
            std::size_t full_valid{};
        };

        void register_switch_socket_result(std::string socket_name,
                                           std::vector<std::pair<std::string, FecSwitchStat>> socket_times)
        {
            switch_socket_records_.emplace_back(std::string{ socket_name }, std::move(socket_times));
        }

        void register_frame_reading_result(std::string name, const FrameReadingStat& stat)
        {
            frame_reading_records_.emplace_back(std::string{ name }, stat);
        }

        void register_task_result(std::string_view task_name, const std::vector<TaskStat>& process_times)
        {
            task_records_.try_emplace(std::string{ task_name }, process_times);
        }

        void register_output_sink_result(std::string_view sink_name, const std::vector<std::size_t>& bytes_written)
        {
            sink_records_.try_emplace(std::string{ sink_name }, bytes_written);
        }

        void register_queue_result(const QueueStat& stat) { queue_record_.second = stat; }

        ~AppReport();

      private:
        std::map<std::string, std::vector<TaskStat>> task_records_;
        std::map<std::string, std::vector<std::size_t>> sink_records_;
        std::vector<std::pair<std::string, std::vector<std::pair<std::string, FecSwitchStat>>>> switch_socket_records_;
        std::vector<std::pair<std::string, FrameReadingStat>> frame_reading_records_;
        std::pair<std::string_view, QueueStat> queue_record_{ std::string_view{ "Counts" }, {} };

        void report_task_result();
        void report_sink_file_result();
        void report_socket_result();
        void report_buffer_result();

        void report_frame_reading_result();
    };
} // namespace srs
