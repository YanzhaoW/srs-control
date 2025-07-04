Command line tools
==================

.. _srs_control:

srs_control
-----------

Synopsis
~~~~~~~~

.. code-block:: bash

    ./srs_control [-p DATA_PRINT_OPTION] [-v LOG_LEVEL] [-h]

Description
~~~~~~~~~~~

:program:`srs_control` is the main program that controls the communications with SRS
system, the data analysis and data writing.

Options
~~~~~~~

.. program:: srs_control

.. option:: -h, --help

    Print the help message

.. option:: -v, --version

    Show the current version

.. option:: --root-version

    Show the ROOT version if used

.. option:: -l, --log-level

    Set the verbose level. Available options: "critical", "error", "warn", "info" (default), "debug", "trace", "off".

.. option:: -p, --print-mode

    Set the data printing mode.

    Available options:

     - speed (default): print the reading rate of received data.

     - header: print the header message of received data.

     - raw: print the received raw bytes.

     - all: print all data, including header, hit and marker data, but no raw data.

.. option:: -o, --output-files

    Set the file outputs (more details below).

.. option:: -c, --config-file

    Set the JSON configuration file (more details below).

.. option:: --dump-config

    Dump the default configuration values to a file (more details below).

Data output to multiple files
+++++++++++++++++++++++++++++

``srs_control`` can output received data into multiple files with different types at the
same time. Currently, following output types are available:

- **binary**

  - raw data if ``.lmd`` or ``.bin``
  - Protobuf data if ``.binpb``

- **json**. File extensions: ``.json`` (NOTE: JSON file could be very large)
- **root**. File extensions: ``.root`` (require ROOT library)
- **UDP socket** (Protobuf + gzip). Input format: ``[ip]:[port]``

Users have to use the correct file extensions to enable the corresponding outputs.

Example
~~~~~~~

To output the same data to multiple different output types at the same time:

.. code-block:: bash

    ./srs_control -o "output1.root" -o "output2.root" \
                  -o "output.bin" -o "output.binpb" \
                  -o "output.json" -o "localhost:9999"

Configuration
-------------

Additional configuration of ``srs_control`` is setup via a JSON file. By default, it
reads the JSON file in the default location ``${HOME}/.config/srs-control/config.json``.
The program will automatically creates the JSON file with default values if the file
doesn't exist. Alternatively, you could also use ``--dump-config`` option to dump all
default values into a file:

.. code-block:: bash

    ./srs_control --dump-config "config.json"

If the file name is not provided, the default configuration values will be written to
the default location.

If needed, the program option ``-c`` could also be used to specify the configuration file instead of the
one in the default location:

.. code-block:: bash

    ./srs_control -c "config.json" -o "output.root"

Configuration values from JSON
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Default values:

.. code-block:: json

    {
        "fec_control_local_port": 6007,
        "fec_data_receive_port": 6006,
        "remote_fec_ips": ["10.0.0.2"]
    }

Explanations:

- ``remote_fec_ips``: IPs of multiple FECs providing data stream. For example, value
  ``["10.0.0.2", "10.0.0.3"]`` means the data stream comes both from two FECs with the
  IP address 10.0.0.2 and 10.0.0.3.

Other tools
-----------

.. toctree::
    :maxdepth: 1

    stubs/check_binpb_message
    stubs/check_udp_message
