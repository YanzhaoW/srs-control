Reading delimited binary data from the binary output file
#############################################################

The class :cpp:class:`srs::reader::RawFrame`, with an alias :cpp:type:`srs::RawFrameReader`, is used to extract frames of binary data separately and consecutively from a binary file (or a FIFO file). Frames in the binary file are divided by a value in the type :cpp:type:`RawDelimSizeType<srs::RawDelimSizeType>` located at the beginning of each frame, which stores the total byte size of the current frame.

This class provides both a public method and a static public method for the users to extract the frames from the binary file. The static public method should be used if the user wants to use the self-provided file handler and data buffer. In such case, the class should be constructed from its default constructor (:cpp:func:`RawFrame::RawFrame()`) and no memory is allocated internally. On the other hand, the normal public method can be used if the user wants to rely on this class for the file opening and the data buffer. In this case, the constructor with a string input should be used instead and the object should be kept alive throughout the whole reading.

**Minimum example:**

.. code-block:: cpp
  :linenos:

  #include <srs/srs.hpp>
  #include <string>
  #include <fstream>

  auto main() -> int
  {
    const auto filename = std::string{ "output.bin" };

    // Using external buffer
    auto raw_frame_reader1 = srs::RawFrameReader{};

    auto input_file = std::ifstream{ filename, std::ios::binary };
    auto external_buffer = std::vector<char>{};

    raw_rame_reader1.read_one_frame(external_buffer, input_file);

    // Using internal buffer
    auto raw_frame_reader2 = srs::RawFrameReader{ filename };

    auto binary_data = raw_frame_reader2.read_one_frame();

    return 0;
  }

Details of :cpp:class:`srs::reader::RawFrame`
=====================================================

.. doxygentypedef:: srs::RawFrameReader
   :project: srs
