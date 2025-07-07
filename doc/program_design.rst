Program Design
==============

The design philosophy of this program follows a low coupling and high coherence of different logical components. This means each block of the program is completely isolated from each other and only performs no more than one single task. Multi-threading is also implemented in the program, in such the way that workloads of different tasks can be evenly distributed across threads **asynchronously**. The technique used to achieve this asynchronous concurrency is the so-called "*task scheduler*". Details about this task scheduling is explained in the section `Data Analysis Workflow`_. The main components, like communications, configurations and analysis, are all aggregated in a single facade class, ``srs::App``, which is the main interface of the library.

General Structure
-----------------

.. image:: /media/general_structure.svg

Program Options
---------------

Application Class
-----------------

UDP Connections
---------------

Data Analysis Workflow
----------------------
