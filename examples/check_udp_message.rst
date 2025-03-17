srs_check_udp
==================

Synopsis
-------------------

.. code-block:: bash

  ./srs_check_udp --port [port number] --ip [ip address]

Description
-------------------

The executable opens up a UDP socket to listen the message sent from :ref:`srs_control <executable:Synopsis>`. Check the `source file <https://github.com/YanzhaoW/srs-control/blob/master/examples/check_udp_message.py>`_ for the full details.

Options
-------------------

.. program:: srs_check_udp

.. option:: --ip

   Specify the ip address of the listening UDP socket.

.. option:: --port

   Specify the port number of the listening UDP socket.

Example
-------------------

.. code-block:: bash

  ./srs_check_udp --port [port number] --ip "localhost"

See the source file `check_udp_message.py <https://github.com/YanzhaoW/srs-control/blob/master/examples/check_udp_message.py>`_.
