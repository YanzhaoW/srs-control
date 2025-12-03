add_library(srs_data SHARED)

target_link_libraries(
    srs_data
    PUBLIC
        $<BUILD_LOCAL_INTERFACE:fmt::fmt>
        $<BUILD_LOCAL_INTERFACE:protobuf::libprotobuf>
    PRIVATE $<BUILD_LOCAL_INTERFACE:Boost::thread>
)

target_sources(
    srs_data
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS ${CMAKE_SOURCE_DIR}/backend ${CMAKE_BINARY_DIR}/backend
            FILES
                DataStructsFormat.hpp
                SRSDataStructs.hpp
                ${CMAKE_CURRENT_BINARY_DIR}/message.pb.h
)

target_sources(srscpp PUBLIC FILE_SET publicHeaders FILES SRSDataStructs.hpp)

protobuf_generate(
    TARGET srs_data
    PROTOS message.proto
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

protobuf_generate(
    TARGET srs_data
    LANGUAGE python
    PROTOS message.proto
    PROTOC_OUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

if(ROOT_FOUND)
    target_link_libraries(srs_data PUBLIC ROOT::RIO ROOT::Tree)
    target_compile_definitions(srs_data PUBLIC HAS_ROOT=1)
    target_compile_options(srs_data PUBLIC "-Wno-cpp")
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/SRSDataStructs.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    root_generate_dictionary(
        srs_dict 
        SRSDataStructs.hpp
        MODULE srs_data
        LINKDEF SRSLinkDef.hpp
    )
endif()
