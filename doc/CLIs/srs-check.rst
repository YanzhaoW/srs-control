###########
 srs-check
###########

**********
 Synopsis
**********

.. code-block:: bash

    ./srs-check [-l] [-h] [log_file.yaml]

*************
 Description
*************

Executable to check the logging from the last run and check whether all functions and coroutines exited correctly. This executable is recommended to diagnose the issues if the :program:`srs-control` was frozen in the last run. The logging file is stored in ``${XDG_STATE_HOME}/srs-control/srs-control.log`` with rotating scheme.

*********
 Options
*********

.. program:: srs-fec-emulator

.. option:: -h, --help

    Print the help message

.. option:: -l, --show-last-log

    Show the log from the last run

.. option:: <file>

    Set the path of the log file. By default, the path is determined by the environment variable ``XDG_STATE_HOME``. If the environment variable is not defined, use `~/.local/state` as the replacement.

    :type: string
    :default: "${XDG_STATE_HOME}/srs-control/srs-control.log"
