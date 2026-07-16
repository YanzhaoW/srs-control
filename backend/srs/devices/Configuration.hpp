#pragma once

#include "srs/utils/CommonDefinitions.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace srs
{
    /**
     * @class Config
     * @brief Main configuration struct
     */
    struct Config
    {
        /**
         * @brief The port number of the local network that is used for the communication to FECs.
         */
        int fec_control_local_port = common::FEC_CONTROL_LOCAL_PORT;

        /**
         * @brief The port number of the remote network that is used for the communication to FECs.
         */
        int fec_control_remote_port = common::DEFAULT_SRS_CONTROL_PORT;

        /**
         * @brief The port number of the local network that is used for reading the data stream from FECs.
         */
        std::vector<int> fec_data_receive_ports = common::FEC_DAQ_RECEIVE_PORT;

        /**
         * @brief The size of data buffer to store the incoming UDP frames.
         */
        std::size_t data_buffer_size = common::LARGE_READ_MSG_BUFFER_SIZE;

        /**
         * @brief Remote IP addresses of FECs.
         */
        std::vector<std::string> remote_fec_ips{ std::string{ common::DEFAULT_SRS_IP } };

        /**
         * @brief Capacity of the buffer queue.
         */
        std::size_t buffer_queue_capacity = common::DEFAULT_DATA_QUEUE_SIZE;

        /**
         * @brief Output file names.
         */
        std::vector<std::string> output_filenames;

        /**
         * @brief Number of splits from the input data to multiple outputs.
         */
        std::size_t output_split = 1;

        /**
         * @brief time (milliseconds) to wait after turning off the srs and before stopping data reading
         */
        std::size_t time_wait_after_acq_off_ms = 1000;

        /**
         * @brief Enable frame counter checking to detect frame drop.
         */
        bool enable_frame_counter_check = false;

        /**
         * @brief Enable warnings when possible data drop occurs.
         */
        bool warn_if_data_drop = false;

        /**
         * @brief Show thread ID from console printouts.
         */
        bool show_thread_id = false;

        /**
         * @brief Set print mode.
         */
        common::DataPrintMode data_print_mode = srs::common::DataPrintMode::print_speed;
    };
} // namespace srs
