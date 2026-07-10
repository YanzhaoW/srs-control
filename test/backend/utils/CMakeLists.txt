target_sources(
    test_utility
    PRIVATE SRSEmulator.cpp SRSWorld.cpp
    PUBLIC FILE_SET HEADERS FILES SRSEmulator.hpp SRSWorld.hpp
)

target_link_libraries(
    test_utility
    PUBLIC
        Taskflow::Taskflow
        asio::asio
        concurrentqueue::concurrentqueue
        fmt::fmt-header-only
        glaze::glaze
        gsl::gsl-lite
        magic_enum::magic_enum
        protobuf::libprotobuf
        spdlog::spdlog
        srs_data
        srscpp
        zpp_bits::zpp_bits
)

