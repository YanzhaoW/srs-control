#pragma once

#include "srs/data/SRSDataStructs.hpp"    // IWYU pragma: export
#include "srs/readers/ProtoMsgReader.hpp" // IWYU pragma: export
#include "srs/readers/RawFrameReader.hpp" // IWYU pragma: export

namespace srs
{
    using ProtoMsgReader = reader::ProtoMsg;
    using RawFrameReader = reader::RawFrame;
} // namespace srs
