#############
 srs-control
#############

**********
 Synopsis
**********

.. code-block:: bash

    ./srs-control [-o FILENAME] [-l LOG_LEVEL] [-h]

*************
 Description
*************

:program:`srs-control` is the main program that controls the communications with SRS system, the data analysis and data writing.

*********
 Options
*********

.. program:: srs-control

.. option:: -h, --help

    Print the help message

.. option:: -v, --version

    Show the current version

.. option:: --root-version

    Show the ROOT version if used

.. option:: -a, --action

    Used to perform an individual action, like switch on or off the FEC devices ("acq-on" or "acq-off")

    :type: string
    :default: "all"

.. option:: -l, --log-level

    Set the verbose level. Available options: "critical", "error", "warn", "info", "debug", "trace", "off".

    :type: string
    :default: "info"

.. option:: -r, --run-time

    Set the run-time of the application in seconds. If the value is larger than 0, the application will stop after the specified time. If the value is zero, the application will keep running until users use "Ctrl-C" to stop the application.

    :type: int
    :default: 0

.. option:: -p, --print-mode

     Set the data printing mode.

     Available options:

      - speed: print the reading rate of received data.

      - header: print the header message of received data.

      - raw: print the received raw bytes.

      - all: print all data, including header, hit and marker data, but no raw data.

    :type: string
    :default: speed

.. option:: -c, --config-file

    Set the JSON configuration file (more details below).

    :type: string
    :default: "~/.config/srs-control/config.yaml"

.. option:: -s, --split-output

    Splitting the read data into multiple sinks of the same type.

    :type: int
    :default: 1

.. option:: --dump-config

    Dump the default configuration values to a file (more details below).

    :type: string
    :default: "~/.config/srs-control/config.yaml"

.. option:: -o, --output-files

    Set the file outputs (more details below).

    :type: list[string]
    :default: []

*******************************
 Data output to multiple files
*******************************

:program:`srs-control` can output received data into multiple files with different types at the same time. Currently, following output types are available:

- **binary**

  - raw data if ``.lmd`` or ``.bin``
  - Protobuf data if ``.binpb``

- **json**. File extensions: ``.json`` (NOTE: JSON file could be very large)
- **root**. File extensions: ``.root`` (require ROOT library)
- **UDP socket** (Protobuf + gzip). Input format: ``[ip]:[port]``
- **UDP socket** (Raw data). Input format: ``[ip]:?[port]``

Users have to use the correct file extensions to enable the corresponding outputs.

*********
 Example
*********

To output the same data to multiple different output types at the same time:

.. code-block:: bash

    ./srs-control -o "output1.root" -o "output2.root" \
                  -o "output.bin" -o "output.binpb" \
                  -o "output.json" -o "localhost:9999"

***************
 Configuration
***************

Additional configuration of :program:`srs-control` is setup via a YAML or JSON file. It's defined in the struct :cpp:class:`srs::Config`.

Default location
================

By default, it reads the config file in the *default location* ``${XDG_CONFIG_HOME}/srs-control/config.yaml``. If the environment variable ``XDG_CONFIG_HOME`` is undefined, ``~/.config/srs-control/config.yaml`` will be chosen instead. The program automatically creates the config file with default values if the file doesn't exist. Or it can be manually generated with:

.. code-block:: bash

    ./srs-control --dump-config

Custom config file location
===========================

Alternatively, you could also use ``--dump-config`` option to dump all default values into a file located in the current folder:

.. code-block:: bash

    ./srs-control --dump-config "config.yaml"

If needed, the program option ``-c`` could also be used to specify the configuration file instead of the one in the default location:

.. code-block:: bash

    ./srs-control -c "config.yaml" -o "output.root"
