Deserialization of the UDP binary data
##########################################

The class :cpp:class:`srs::ProtoMsgReader` is responsible for the deserialization of the UDP binary data, which converts the Protobuf binary data sent by a UDP socket to a C++ data structure. All memory allocation of this class is done in its constructor. If the conversion are repeated many times, the object should be kept alive to reduce the memory allocation.

The full example can be found in this `script <https://github.com/YanzhaoW/srs-control/blob/master/examples/readUDP/main.cpp>`_ .

**Minimum example:**

.. code-block:: cpp
  :linenos:

  #include <srs/srs.hpp>

  auto main() -> int
  {
    auto msg_reader = srs::ProtoMsgReader{};

    std::string_view proto_binary = get_proto_msg();

    const auto& data_struct = msg_reader.convert(proto_binary);

    return 0;
  }

Details of :cpp:class:`srs::ProtoMsgReader`
==============================================

.. doxygenclass:: srs::ProtoMsgReader
   :project: srs
   :members:
   :undoc-members:
   :private-members:
