#pragma once

#include <srs/utils/CommonDefinitions.hpp>
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
         * @brief The port number of the local network that is used for reading the data stream from FECs.
         */
        int fec_data_receive_port = common::FEC_DAQ_RECEIVE_PORT;

        /**
         * @brief Remote IP addresses of FECs.
         */
        std::vector<std::string> remote_fec_ips{ std::string{ common::DEFAULT_SRS_IP } };
    };
} // namespace srs
