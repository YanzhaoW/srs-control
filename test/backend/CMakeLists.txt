enable_testing()

add_executable(srs_backend_test)
target_sources(srs_backend_test PRIVATE test_main.cpp)
target_link_libraries(
    srs_backend_test
    PRIVATE GTest::gtest_main
            spdlog::spdlog
            zpp_bits::zpp_bits
            Boost::thread
            protobuf::libprotobuf
            magic_enum::magic_enum
            srscpp
            srs_data
            gsl::gsl-lite
            fmt::fmt)

target_include_directories(srs_backend_test PRIVATE ${CMAKE_SOURCE_DIR}/test/backend
                                                    ${CMAKE_SOURCE_DIR}/backend)

add_subdirectory(srs)
