#pragma once

#include <srs/data/SRSDataStructs.hpp>
#include <srs/readers/ProtoMsgReader.hpp>
#include <srs/readers/RawFrameReader.hpp>

namespace srs
{
    using ProtoMsgReader = reader::ProtoMsg;
    using RawFrameReader = reader::RawFrame;
} // namespace srs
