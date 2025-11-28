target_sources(
    test_utility
    PRIVATE SRSEmulator.cpp SRSWorld.cpp
    PUBLIC FILE_SET
           HEADERS
           FILES
           SRSEmulator.hpp
           SRSWorld.hpp)

target_link_libraries(
    test_utility
    PUBLIC srscpp
           srs_data
           Boost::headers
           fmt::fmt
           zpp_bits::zpp_bits
           magic_enum::magic_enum
           protobuf::libprotobuf
           glaze::glaze
           Taskflow::Taskflow
           gsl::gsl-lite
           spdlog::spdlog)

target_sources(unit_test_srs_backend PRIVATE UnitTestWriter.cpp)
target_link_libraries(unit_test_srs_backend PRIVATE GTest::gtest_main test_utility)
