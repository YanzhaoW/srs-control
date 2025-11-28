#include "srs/converters/DataConvertOptions.hpp"
#include "srs/writers/BinaryFileWriter.hpp"
#include "srs/writers/JsonWriter.hpp"
#include "srs/writers/UDPWriter.hpp"
#include "srs/writers/WriterConcept.hpp"
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/thread_pool.hpp>
#include <gtest/gtest.h>

namespace writer = srs::writer;
namespace process = srs::process;
namespace asio = boost::asio;

using enum process::DataConvertOptions;

TEST(binary_writer, constructor)
{
    writer::WritableFile auto binary_writer = writer::BinaryFile{ "unit_test.bin", raw_frame, 1 };
}

TEST(JSON_writer, constructor)
{
    writer::WritableFile auto json_writer = writer::Json{ "unit_test.json", structure, 1 };
}

TEST(udp_writer, constructor)
{
    auto io_context = asio::thread_pool{ 1 };
    auto remote_endpoint = asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 };

    writer::WritableFile auto udp_writer = writer::UDP{ io_context, remote_endpoint, 1, raw };
}

#ifdef HAS_ROOT
TEST(ROOT_writer, constructor) {}
#endif
