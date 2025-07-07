target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            FILES
            CommonConcepts.hpp
            CommonDefinitions.hpp
            CommonFunctions.hpp
            ValidData.hpp)

target_sources(srscpp PUBLIC FILE_SET publicHeaders FILES AppStatus.hpp)
