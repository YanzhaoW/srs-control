#include "StructSerializer.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include <cstddef>
#include <vector>

namespace srs::process
{
    StructSerializer::StructSerializer(size_t n_lines)
        : ConverterTask{ "Struct deserializer", none, n_lines }
    {
        output_data_.resize(n_lines);
    }

    void StructSerializer::convert([[maybe_unused]] const StructData* input, [[maybe_unused]] std::vector<char>& output)
    {
        // NOTE: DO NOT do any memory allocation here. If heap data is needed, please initialize it in the constructor.
        output.clear();
    }
} // namespace srs::process
