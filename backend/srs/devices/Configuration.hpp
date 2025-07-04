#pragma once

#include <srs/utils/CommonDefitions.hpp>

namespace srs
{
    struct Config
    {
        int fec_control_local_port = common::FEC_CONTROL_LOCAL_PORT;
        int fec_data_receive_port = common::FEC_DAQ_RECEIVE_PORT;
        std::vector<std::string> remote_fec_ips{ std::string{ srs::common::DEFAULT_SRS_IP } };
    };
} // namespace srs
