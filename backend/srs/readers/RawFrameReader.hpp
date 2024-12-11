#pragma once

#include <fstream>
#include <print>
#include <srs/utils/CommonDefitions.hpp>
#include <string>
#include <vector>

namespace srs::reader
{
    class RawFrame
    {
      public:
        //! Default Constructor. No memory allocation.
        explicit RawFrame() = default;

        /**
         * \brief Constructor that opens a file with the given filename.
         *
         * @param filename The file name of the input binary
         *
         */
        explicit RawFrame(const std::string& filename);

        /**
         *
         * \brief Read one frame of the bianry data to a vector.
         *
         * The input vector is first reserved with #srs::common::LARGE_READ_MSG_BUFFER_SIZE elements and then resized with the
         * \a size value, which is read from the binary file in the beginning. The vector is then set with \a size bytes
         * of data from the file.
         *
         * @param binary_data The vector of chars to be filled.
         * @param input_file Input file handler.
         * @return The amount of bytes read from the binary file. If the binary input file is not open, return 0.
         */
        static auto read_one_frame(std::vector<char>& binary_data, std::ifstream& input_file) -> std::size_t;

        /**
         *
         * \brief Read one frame of the binary data from a file specified by the constructor \ref RawFrame(const
         * std::string&).
         *
         * The binary data frame is written to the internal member variable #input_buffer_;
         * #srs::common::LARGE_READ_MSG_BUFFER_SIZE bytes of memory will be reserved for the vector.
         *
         * @return Non-owning binary data.
         *
         */
        auto read_one_frame() -> std::string_view;

        //! Getter to the internal binary data used by the member function \ref read_one_frame().
        auto get_binary_data() const -> const auto& { return input_buffer_; }

      private:
        std::string input_filename_;     //!< Input binary file name.
        std::ifstream input_file_;       //!< Input binary file handler
        std::vector<char> input_buffer_; //!< internal binary data buffer
    };
} // namespace srs::reader
