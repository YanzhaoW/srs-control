target_sources(
    srscpp
    PRIVATE AppReport.cpp
    PRIVATE
        FILE_SET privateHeaders
            FILES
                AppReport.hpp
                CommonConcepts.hpp
                CommonDefinitions.hpp
                CommonFunctions.hpp
                EnumConvertFunctions.hpp
                GlazeCustom.hpp
                UDPFormatters.hpp
                ValidData.hpp
)
