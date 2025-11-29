#include "RawToDelimRawConveter.hpp"
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>
#include <zpp_bits.h>

namespace srs::process
{
    void Raw2DelimRawConverter::convert(std::string_view input, std::vector<char>& output)
    {
        auto size = static_cast<SizeType>(input.size());
        output.reserve(size + sizeof(size));
        auto deserialize_to = zpp::bits::out{ output, zpp::bits::append{}, zpp::bits::endian::big{} };
        deserialize_to(size, zpp::bits::unsized(input)).or_throw();
    }

} // namespace srs::process
