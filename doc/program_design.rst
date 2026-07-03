################
 Program Design
################

.. contents:: Contents
    :depth: 3

The design philosophy of this program follows a low coupling and high coherence of different logical components. This means each block of the program is completely isolated from each other and only performs no more than one single task. Multi-threading is also implemented in the program, in such the way that workloads of different tasks can be evenly distributed across threads via a "*task scheduler*". Details about this task scheduling is explained in the section `Data Analysis Workflow`_. The main components, like communications, configurations and analysis, are all aggregated in a single facade class, :cpp:class:`srs::App`, which is the main interface of the library.

*******************
 General Structure
*******************

.. figure:: /media/general_structure.svg
    :align: center

    Figure 1: General structure of srs-control program.

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

Application class :cpp:class:`srs::App` is the main entrance point of ``srs-control`` program and its member variables represent different components of the program: task workflows, network connections with FECs and writing and reading of the application configurations, etc.

*****************
 UDP Connections
*****************

************************
 Data Analysis Workflow
************************

Buffer and buffer queue
=======================

Buffer queue is essential for the program to read the data from UDP sockets and consume data asynchronously. The buffer queue has the type :cpp:class:`srs::BufferQueue`, which contains a vector of buffer elements of the type :cpp:class:`srs::LargeBuffer`. The simple way of managing the insert and poping of the buffer element would be copying the buffer element, stored in the :cpp:class:`srs::connection::DataSocket`, into the buffer queue, and later pop any available buffer inside the buffer queue to the buffer elements stored in :cpp:class:`srs::workflow::TaskDiagram` for the data analysis taskflow. However, this process would be really slow due to the copying the constantly memory allocation of the read buffer in the data socket. Instead of copying the whole buffer into the buffer queue, a much more efficient way is to create an empty buffer first in the buffer queue and immediately swap it with the buffer from the data socket. But again, the data socket buffer is empty and needs memory allocation again for the UDP reading. This can be solved by using another buffer queue to store the all the used buffer from the :cpp:class:`srs::workflow::TaskDiagram` and swap it with the data socket buffer. Therefore, throughout the whole reading and analysis process, no memory allocation occurs.

.. figure:: /media/buffer_queue.svg
    :align: center

    Figure 2: The push and pop of the buffer queue.

The interactions between the buffer queue and the UDP sockets is completely asynchronous with the interactions between the buffer queue and analysis taskflows. Regarding to the former one, it can be described chronically as the following:

1. UDP socket receives data from the remote server, which is stored in its buffer of the type ``LargeBuffer`` along with a data size of the received message.
2. According to the given size, the ``resize()`` member function is called, which just changes size of the buffer, but without changing any values outside of its size.
3. The (non-const) reference of the socket buffer is given to the buffer queue.
4. The socket buffer is pushed to the **valid buffer queue** by first creating an empty buffer with both the size and capacity to be 0 in place of the tail of the queue and swapping the new empty buffer with the socket buffer, which contains the valid message. The swapping action is just the exchange of the internal data addresses of the two buffer.
5. If the valid buffer queue returns ``false`` from the call of ``try_emplace``, it means the valid buffer queue is full and data loss will be reported. In such case, the swapping action fails and the socket buffer is non-empty and given directly back the to socket.
6. If the valid buffer queue is not full. The socket buffer becomes empty and filled with a buffer from the **trash buffer queue**. Similar to the pushing process, the filling process swaps the non-empty head buffer of the queue with the empty socket buffer. The non-empty socket buffer is then given back to the socket.
7. Goes back to step 1 when new data is received from the UDP socket.

On the other hand, interaction between the buffer queue and the analysis taskflow can be described as following:

1. The input buffer of the taskflow is initialized when the taskflow is constructed. The initialization allocates the input buffer with maximal number (``buffer_size``) of zero values.
2. Once the taskflow starts, it pushes the input buffer to the tail of the trash buffer queue.
3. After the pushing, the input buffer becomes empty and then filled with the head buffer of the valid buffer queue.
4. The non-empty input buffer is then given to the analysis taskflow.
5. Goes back to step 1 once the current turn of the analysis taskflow is finished.
