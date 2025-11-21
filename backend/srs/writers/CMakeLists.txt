target_sources(srscpp PRIVATE DataWriter.cpp BinaryFileWriter.cpp JsonWriter.cpp UDPWriter.cpp)
target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            FILES
            BinaryFileWriter.hpp
            DataWriter.hpp
            DataWriterOptions.hpp
            JsonWriter.hpp
            RootFileWriter.hpp
            UDPWriter.hpp)

target_link_libraries(srscpp PRIVATE $<BUILD_LOCAL_INTERFACE:glaze::glaze>)
target_include_directories(srscpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
