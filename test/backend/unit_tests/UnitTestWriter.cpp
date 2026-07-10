#include "srs/converters/DataConvertOptions.hpp"
#include "srs/writers/BinaryFileWriter.hpp"
#include "srs/writers/JsonWriter.hpp"
#include "srs/writers/UDPWriter.hpp"
#include "srs/writers/WriterConcept.hpp"
#include <asio/ip/udp.hpp>
#include <asio/thread_pool.hpp>
#include <catch2/catch_test_macros.hpp>

namespace writer = srs::writer;
namespace process = srs::process;

using enum process::DataConvertOptions;

TEST_CASE("binary_writer")
{
    writer::WritableFile auto binary_writer = writer::BinaryFile{ "unit_test.bin", raw_frame, 1 };
}

TEST_CASE("JSON_writer") { writer::WritableFile auto json_writer = writer::Json{ "unit_test.json", structure, 1 }; }

TEST_CASE("udp_writer")
{
    auto io_context = asio::thread_pool{ 1 };
    auto remote_endpoint = asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 };

    writer::WritableFile auto udp_writer = writer::UDP{ io_context, remote_endpoint, 1, raw };
}
