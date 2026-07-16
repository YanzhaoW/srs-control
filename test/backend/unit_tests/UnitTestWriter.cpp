#include "srs/converters/DataConvertOptions.hpp"
#include "srs/sinks/BinaryFileWriter.hpp"
#include "srs/sinks/JsonWriter.hpp"
#include "srs/sinks/UDPWriter.hpp"
#include "srs/sinks/WriterConcept.hpp"
#include <asio/ip/udp.hpp>
#include <asio/thread_pool.hpp>
#include <catch2/catch_test_macros.hpp>

namespace sink = srs::sink;
namespace process = srs::process;

using enum process::DataConvertOptions;

TEST_CASE("binary_writer")
{
    sink::WritableFile auto binary_writer = sink::BinaryFile{ "unit_test.bin", raw_frame, 1 };
}

TEST_CASE("JSON_writer") { sink::WritableFile auto json_writer = sink::Json{ "unit_test.json", structure, 1 }; }

TEST_CASE("udp_writer")
{
    auto io_context = asio::thread_pool{ 1 };
    auto remote_endpoint = asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 };

    sink::WritableFile auto udp_writer = sink::UDP{ io_context, remote_endpoint, 1, raw };
}
