#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace srs::workflow
{
    class TaskReport
    {
      public:
        struct Stat
        {
            uint64_t total_time{};
            uint64_t total_sample_size{};
        };

        void register_task_result(std::string_view task_name, const std::vector<Stat>& process_times)
        {
            records_.try_emplace(std::string{ task_name }, process_times);
        }

        ~TaskReport();

      private:
        std::map<std::string, std::vector<Stat>> records_;
    };
} // namespace srs::workflow
