target_sources(
    srscpp
    PRIVATE
        BinaryFileWriter.cpp
        Manager.cpp
        FrameCountChecker.cpp
        JsonWriter.cpp
        RootFileWriter.cpp
        UDPWriter.cpp
)
target_sources(
    srscpp
    PRIVATE
        FILE_SET privateHeaders
            FILES
                BinaryFileWriter.hpp
                Manager.hpp
                DataWriterOptions.hpp
                FrameCountChecker.hpp
                JsonWriter.hpp
                RootFileWriter.hpp
                UDPWriter.hpp
)

target_link_libraries(srscpp PRIVATE $<BUILD_LOCAL_INTERFACE:glaze::glaze>)
target_include_directories(srscpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
