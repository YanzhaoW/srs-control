#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <string_view>

namespace srs::process
{
    enum class DataConvertOptions : uint8_t
    {
        none,
        raw,
        raw_frame,
        structure,
        proto,
        proto_frame
    };

    constexpr auto convert_option_to_string(DataConvertOptions option) -> std::string_view
    {
        using enum DataConvertOptions;
        switch (option)
        {
            case none:
                return std::string_view{ "none" };
            case raw:
                return std::string_view{ "raw" };
            case raw_frame:
                return std::string_view{ "raw_frame" };
            case structure:
                return std::string_view{ "structure" };
            case proto:
                return std::string_view{ "proto" };
            case proto_frame:
                return std::string_view{ "proto_frame" };
        }
    }

    struct ConvertOptionRelation
    {
        constexpr ConvertOptionRelation(DataConvertOptions option, DataConvertOptions next_option)
            : dependee{ option }
            , depender{ next_option }
        {
        }
        DataConvertOptions dependee;
        DataConvertOptions depender;
    };

    constexpr auto EMPTY_CONVERT_OPTION_COUNT_MAP = []()
    {
        using enum DataConvertOptions;
        return std::array{ std::make_pair(raw, 0),
                           std::make_pair(raw_frame, 0),
                           std::make_pair(structure, 0),
                           std::make_pair(proto, 0),
                           std::make_pair(proto_frame, 0) };
    }();

    constexpr auto CONVERT_OPTION_RELATIONS = []()
    {
        using enum DataConvertOptions;
        return std::array{ ConvertOptionRelation{ raw, raw_frame },
                           ConvertOptionRelation{ raw, structure },
                           ConvertOptionRelation{ structure, proto },
                           ConvertOptionRelation{ structure, proto_frame } };
    }();

    constexpr auto convert_option_has_dependency(DataConvertOptions dependee, DataConvertOptions depender) -> bool
    {
        // fmt::println("-----------dependee: {}, depender: {}",
        //              convert_option_to_string(dependee),
        //              convert_option_to_string(dependee));
        if (dependee == depender)
        {
            return true;
        }

        return std::ranges::any_of(CONVERT_OPTION_RELATIONS,
                                   [dependee, depender](ConvertOptionRelation relation) -> bool
                                   {
                                       if (dependee == relation.dependee)
                                       {
                                           if (depender == relation.depender)
                                           {
                                               return true;
                                           }
                                           return convert_option_has_dependency(relation.depender, depender);
                                       }

                                       // nothing is depending on it
                                       return false;
                                   });
    }

} // namespace srs

template <>
class fmt::formatter<srs::process::DataConvertOptions>
{
  public:
    static constexpr auto parse(format_parse_context& ctx) { return ctx.end(); }
    template <typename FmtContent>
    constexpr auto format(srs::process::DataConvertOptions option, FmtContent& ctn) const
    {
        using enum srs::process::DataConvertOptions;
        switch (option)
        {
            case none:
                return fmt::format_to(ctn.out(), "none");
            case raw:
                return fmt::format_to(ctn.out(), "raw");
            case raw_frame:
                return fmt::format_to(ctn.out(), "raw_frame");
            case structure:
                return fmt::format_to(ctn.out(), "structure");
            case proto:
                return fmt::format_to(ctn.out(), "proto");
            case proto_frame:
                return fmt::format_to(ctn.out(), "proto_frame");
        }
    }
};
