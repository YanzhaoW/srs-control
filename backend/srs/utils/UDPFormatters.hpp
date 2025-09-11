#pragma once

#include <fmt/base.h>

#include "srs/connections/ConnectionTypeDef.hpp"

template <>
class fmt::formatter<srs::connection::UDPEndpoint>
{
  public:
    static constexpr auto parse(format_parse_context& ctx) { return ctx.end(); }
    template <typename FmtContent>
    constexpr auto format(const srs::connection::UDPEndpoint& endpoint, FmtContent& ctn) const
    {
        return fmt::format_to(ctn.out(), "{}:{}", endpoint.address().to_string(), endpoint.port());
    }
};
