################
 Program Design
################

The design philosophy of this program follows a low coupling and high coherence of different logical components. This means each block of the program is completely isolated from each other and only performs no more than one single task. Multi-threading is also implemented in the program, in such the way that workloads of different tasks can be evenly distributed across threads via a "*task scheduler*". Details about this task scheduling is explained in the section `Data Analysis Workflow`_. The main components, like communications, configurations and analysis, are all aggregated in a single facade class, ``srs::App``, which is the main interface of the library.

*******************
 General Structure
*******************

.. image:: /media/general_structure.svg

*****************
 Program Options
*****************

The ``srs_control`` program accepts user options in two different ways: JSON configuration file and command line options. Both are defined in the `main.cpp <https://github.com/YanzhaoW/srs-control/blob/master/backend/main.cpp>`_ file independently from the ``App`` class. **Options specified from the JSON file can be overwritten if a same option also exists in the command line.** Thus, it's recommended to store rarely changed options in the JSON file, such as the port number of the remote FEC devices or local data stream socket. On the other hand, command line options usually include frequently changed options, like output file names or the log level.

Command Line Options
====================

A popular command line interface library, `CLI11 <https://github.com/CLIUtils/CLI11>`_, is used to set the multiple parameter variables according to the options given by the user. Some simple variables, like ``n_output_split``, is set directly by the ``add_option`` function call:

.. code-block:: cpp

    auto n_output_split = 1;
    cli_args.add_option("-s, --split-output", n_output_split, "Splitting the output data into different files.")
             ->capture_default_str();

Other variables, typically enumerators, require a mapping passed to the ``add_option``:

.. code-block:: cpp

    cli_args.add_option(
                 "-a, --action",
                 action_mode_enum,
                 fmt::format("Set the action of program\nAvailable options: [{}]", fmt::join(ACTION_MODE_NAMES, ", ")))
             ->transform(CLI::CheckedTransformer(action_mode_map, CLI::ignore_case).description(""))
             ->default_str(get_enum_dashed_name(action_mode_enum));

The `action_mode_map` is constant global variable in the type `std::map<std::string, common::ActionMode>`.

Furthermore, options, like ``--dump-config``, can set more than one variables at the same time. This is done by passing a lambda to the ``add_option``:

.. code-block:: cpp

    auto dump_config_callback = [&json_filepath, &is_dump_needed](const std::string& filename)
         {
             is_dump_needed = true;
             json_filepath = filename;
         };
    cli_args.add_option_function<std::string>(
                "--dump-config", dump_config_callback, "Dump default configuration to the file")
            ->expected(0, 1);

JSON Configuration
==================

*******************
 Application Class
*******************

Application class `App` is the main entrance point of `srs-control` program and its member variables represent different components of the program: task workflows, network connections with FECs and writing and reading of the application configurations, etc. 




*****************
 UDP Connections
*****************

************************
 Data Analysis Workflow
************************
