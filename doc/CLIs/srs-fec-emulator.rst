##################
 srs-fec-emulator
##################

**********
 Synopsis
**********

.. code-block:: bash

    ./srs-fec-emulator [-l LOG_LEVEL] [-h] config_file.yaml

*************
 Description
*************

:program:`srs-fec-emulator` is an executable that emulates multiple FECs with different IP addresses. Once the application is launched, FECs wait for connections by listening the port specified by :option:`port <srs-fec-emulator FECs[].port>`. Different actions are then performed depending on the message received:

- ``acq-on``: FEC starts to send binary data frame back to the IP address sending the received message with port number specified by :option:`remote_data_port <srs-fec-emulator FECs[].remote_data_port>`.
- ``acq-off``: FEC stops sending the binary data and wait for another `acq-on` message again.

The binary data in the frame is from the serialized :cpp:class:`srs::StructData` object. The :cpp:class:`srs::ReceiveDataHeader` part of the data is set with the ``frame_counter`` from the global incrementing frame_counter, ``udp_timestamp`` from the Unix time from Epoch.

*********
 Options
*********

.. program:: srs-fec-emulator

.. option:: -h, --help

    Print the help message

.. option:: -l, --log-level

    Set the verbose level. Available options: "critical", "error", "warn", "info", "debug", "trace", "off".

    :type: string
    :default: "info"

.. option:: --dump-config

    Dump the default configuration values to a file.

    :type: string
    :default: "fec_emulator.yaml"

.. option:: <config_file>

    Set the configuration file.

    :type: string
    :default: "fec_emulator.yaml"

***************
 Configuration
***************

Default YAML config:

.. code-block:: yaml

    frame_wait_time_ns: 1000
    n_threads: 4
    non_stop: true
    FECs:
      - ip: '127.0.0.1'
        port: 6600
        is_sent_data_only: false
        remote_data_port: 6006
        n_hits:
          min: 0
          max: 10
        n_markers:
          min: 0
          max: 10

.. option:: frame_wait_time_ns

    Waiting time in nano-seconds before sending another frame of data to :program:`srs-control`.

    :type: int
    :default: 1000

.. option:: n_threads

    Number of threads in ``asio::thread_pool``. It's recommended to use the number equal or larger than the number of FEC devices.

    :type: int
    :default: 4

.. option:: non_stop

    Enable non-stop mode if true, which continuously accepts connections from :program:`srs-control` and keep the program running forever. If false, the program will exit once "acq-off" is received.

    :type: bool
    :default: true

.. option:: non_stop

    Individual configurations for each FEC device. The number of FECs enabled is equal to the number of elements specified.

    :type: list[object]

.. option:: FECs[].ip

    IP address of the emulated FEC device. Notice that ip address string should be enclosed by single quotes.

    :type: string

.. option:: FECs[].port

    Port number used by the FEC device to receive connections from :program:`srs-control`.

    :type: int

.. option:: FECs[].is_sent_data_only

    Set the FEC device only to send binary data without accepting from or responding to :program:`srs-control`.

    :type: bool
    :default: false

.. option:: FECs[].remote_data_port

    The port number, to which the serialized data is sent.

    :type: int
    :default: 6006

.. option:: FECs[].n_hits

    Set the minimum and maximum of number hits generated per frame. The number of hits is determined by poolin g the uniform ditribution of [min, max].

    :type: {min, max}

.. option:: FECs[].n_markers

    Set the minimum and maximum of number markers generated per frame. The number of markers is determined by poolin g the uniform ditribution of [min, max].

    :type: {min, max}
