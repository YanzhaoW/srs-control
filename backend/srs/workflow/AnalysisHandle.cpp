#include "srs/workflow/AnalysisHandle.hpp"
#include "srs/Application.hpp"
#include "srs/data/BufferQueue.hpp"
#include "srs/data/DataStructsFormat.hpp"
#include "srs/data/LargeBuffer.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/workflow/DataMonitor.hpp"
#include "srs/workflow/FrameMissMonitor.hpp"
#include "srs/workflow/TaskDiagram.hpp"
#include <asio/any_io_executor.hpp>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

namespace srs::workflow
{

    AnalysisHandle::AnalysisHandle(App* control, std::size_t n_lines)
        : is_data_drop_warn_{ control->get_config().warn_if_data_drop }
        , n_lines_{ n_lines }
        , app_{ control }
        , monitor_{ this, &(control->get_io_context()) }
        , buffer_queue_{ BufferQueue{ { .buffer_size = app_->get_config().data_buffer_size,
                                        .reserve_size = control->get_config().buffer_queue_capacity / 2,
                                        .queue_capacity = control->get_config().buffer_queue_capacity } } }
    {
        spdlog::debug("Handler: Setting the capacity of the buffer queue to {}",
                      control->get_config().buffer_queue_capacity);
        if (app_->get_config().enable_frame_counter_check)
        {
            writers_.enable_frame_count_checker();
        }
    }

    void AnalysisHandle::print_statistics()
    {

        auto& report = app_->get_report();

        task_diagram_->register_report(report);

        auto frame_stat = AppReport::FrameReadingStat{};

        frame_stat.total_time_ns = total_time_ns_;
        frame_stat.total_bytes_read = total_read_data_bytes_;
        frame_stat.n_frames = total_frame_counts_;
        report.register_frame_reading_result("Workflow", frame_stat);

        buffer_queue_.register_report(report);
    }

    void AnalysisHandle::start(asio::any_io_executor executor)
    {
        auto _ = ExitLogger{};
        is_stopped_.store(false);
        task_diagram_ = std::make_unique<TaskDiagram>(*this, n_lines_);

        const auto& app_config = app_->get_config();

        const auto* frame_count_checker = writers_.get_frame_count_checker();
        if (app_config.enable_frame_counter_check and frame_count_checker != nullptr)
        {

            frame_counter_monitor_ = std::make_unique<FrameMissMonitor>(
                FrameMissMonitor::Config{ .sample_size = 200, .sample_time_ms = 10 }, *frame_count_checker);
            frame_counter_monitor_->launch(executor);
        }

        if (print_mode_ == print_speed)
        {
            monitor_.start();
        }

        cv_.notify_all();
        spdlog::trace("Analysis workflow: entering workflow loop");

        task_diagram_->construct_taskflow_and_run(buffer_queue_, is_stopped_);
        spdlog::trace("Analysis workflow: workflow loop ended");
    }

    AnalysisHandle::~AnalysisHandle()
    {
        // stop();
        spdlog::debug("Analysis workflow: total read data bytes: {} ", total_read_data_bytes_.load());
        spdlog::debug("Analysis workflow: total frame counts: {} ", total_frame_counts_.load());
    }
    // DataProcessor::~DataProcessor() = default;

    void AnalysisHandle::stop_workflow()
    {
        auto _ = ExitLogger{};
        // stop may be called before start, where the task_diagram_ is constructed. To prevent task_diagram_ to be
        // nullptr, wait for a condition variable, notified by the start function.
        auto lock = std::unique_lock{ cv_mutex_ };
        cv_.wait(lock, [this]() { return task_diagram_ != nullptr; });

        // CAS operation to guarantee the thread safety
        auto expected = false;
        spdlog::trace("Analysis workflow: trying to stop ... ");
        spdlog::trace("Analysis workflow: current is_stopped status: {}", is_stopped_.load());
        if (is_stopped_.compare_exchange_strong(expected, true))
        {
            const auto _ = ExitLogger{ "scope" };

            while (not task_diagram_->is_taskflow_abort_ready())
            {
                // spdlog::trace("waiting for task diagram to be abort ready");
            }
            spdlog::trace("Analysis workflow: taskflow is ready to abort.");
            buffer_queue_.enqueue_empty(task_diagram_->get_n_lines());
        }
    }

    auto AnalysisHandle::get_data_workflow() const -> const TaskDiagram&
    {
        assert(task_diagram_ != nullptr);
        return *task_diagram_;
    }

    void AnalysisHandle::read_data_once(LargeBuffer& read_data, BufferQueue::Token& token)
    {
        const auto data_size = read_data.get_size();
        total_read_data_bytes_ += data_size;
        ++total_frame_counts_;

        time_point_ = clock_.now();
        auto is_success = buffer_queue_.enqueue(read_data, token);
        total_time_ns_ += static_cast<uint64_t>((clock_.now() - time_point_).count());

        if (not is_success)
        {
            total_drop_data_bytes_ += data_size;
            if (is_data_drop_warn_)
            {
                spdlog::warn("Data drop as the buffer queue is full: Current size/capacity: {}/{}.",
                             buffer_queue_.size(),
                             buffer_queue_.capacity());
            }
        }
    }
} // namespace srs::workflow
