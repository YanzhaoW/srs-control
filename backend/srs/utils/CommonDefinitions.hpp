#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace srs::common
{
    constexpr auto BYTE_BIT_LENGTH = 8;

    // Connections:
    constexpr auto DEFAULT_SRS_IP = std::string_view{ "10.0.0.2" };
    constexpr auto DEFAULT_TIMEOUT_SECONDS = 2;
    constexpr auto WRITE_COMMAND_BITS = uint8_t{ 0xaa };
    constexpr auto DEFAULT_TYPE_BITS = uint8_t{ 0xaa };
    constexpr auto DEFAULT_CHANNEL_ADDRE = uint16_t{ 0xff };
    constexpr auto COMMAND_LENGTH_BITS = uint16_t{ 0xffff };
    constexpr auto ZERO_UINT16_PADDING = uint16_t{};
    constexpr auto SMALL_READ_MSG_BUFFER_SIZE = 96;
    constexpr auto LARGE_READ_MSG_BUFFER_SIZE = 80'000'000; //!< size of the data array for reading a UDP package

    constexpr auto DEFAULT_CMD_LENGTH = uint16_t{ 0xffff };
    constexpr auto CMD_TYPE = uint8_t{ 0xaa };
    constexpr auto WRITE_CMD = uint8_t{ 0xaa };
    constexpr auto READ_CMD = uint8_t{ 0xbb };
    constexpr auto I2C_ADDRESS = uint16_t{ 0x0042 };  /* device address = 0x21 */
    constexpr auto NULL_ADDRESS = uint16_t{ 0x000f }; /* device address = 0x21 */

    constexpr auto INIT_COUNT_VALUE = uint32_t{ 0x80000000 };
    constexpr auto DEFAULT_STATUS_WAITING_TIME_SECONDS = std::chrono::seconds{ 5 };

    // port numbers:
    constexpr auto DEFAULT_SRS_CONTROL_PORT = 6600;
    constexpr auto FEC_DAQ_RECEIVE_PORT = 6006;

    /**
     * @brief Default value of the listening port number used for the FEC communications
     */
    static constexpr int FEC_CONTROL_LOCAL_PORT = 6007;

    // Data processor:
    constexpr auto DEFAULT_DISPLAY_PERIOD = std::chrono::milliseconds{ 200 };
    constexpr auto DEFAULT_ROOT_HTTP_SERVER_PERIOD = std::chrono::milliseconds{ 1000 };
    constexpr auto FEC_ID_BIT_LENGTH = 8;
    constexpr auto HIT_DATA_BIT_LENGTH = 48;
    constexpr auto SRS_TIMESTAMP_HIGH_BIT_LENGTH = 32;
    constexpr auto SRS_TIMESTAMP_LOW_BIT_LENGTH = 10;
    constexpr auto FLAG_BIT_POSITION = 15; // zero based
    constexpr auto GZIP_DEFAULT_COMPRESSION_LEVEL = 9;
    constexpr auto PROTOBUF_ENABLE_GZIP = true;
    constexpr auto DEFAULT_DATA_QUEUE_SIZE = 100;

    /**
     * @enum DataPrintMode
     * @brief Print mode of the status line
     */
    enum class DataPrintMode : uint8_t
    {
        print_speed,  //!< Print the data reading rate
        print_header, //!< Print the header of the data structure
        print_raw,    //!< Print the raw binary data
        print_all     //!< Print everything
    };

    enum class ActionMode : uint8_t
    {
        all,
        acq_on,
        acq_off,
        read_only,
        none,
    };

    using RawDelimSizeType = uint32_t;
} // namespace srs::common
