target_sources(
    srscpp
    PRIVATE
        FILE_SET privateHeaders
            FILES
                CommonConcepts.hpp
                CommonDefinitions.hpp
                CommonFunctions.hpp
                EnumConvertFunctions.hpp
                GlazeCustom.hpp
                UDPFormatters.hpp
                ValidData.hpp
)
