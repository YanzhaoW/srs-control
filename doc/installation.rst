#####################
 Install from source
#####################

***************
 Prerequisites
***************

- C++ compiler (choose either one below)

  - gcc ≥ 15
  - clang ≥ 20

- ROOT ≥ 6.32 (*optional*)
- oneTBB
- CMake ≥ 3.28
- Conan ≥ 2.8.0
- nodejs ≥ 22.9.0 (*optional*)

.. warning::

    DO NOT USE Conda to install ROOT. Either install ROOT from your system package mamanger or build and install it from the source.

***********************
 Install prerequisites
***********************

MacOS
=====

Use `Homebrew <https://brew.sh>`_ to install the prerequisites:

.. code-block:: bash

    sudo brew update
    sudo brew install llvm cmake conan tbb nodejs root

Fedora 41
=========

.. code-block:: bash

    sudo dnf update
    sudo dnf install -y cmake clang tbb-devel root conan nodejs

Debian-based Linux (Debian, Ubuntu, Mint, etc.)
===============================================

Since Debian-based distros only provide outdated software, installing the prerequisites software for Debian based distros is the most complicated. Therefore, it's **highly recommended** to use the pre-built package from the :ref:`download_step:Download the project`.

Except :program:`oneTBB`, most programs have to be installed either from source and some can be installed from a third-party package manager like :program:`Conda`.

To install :program:`oneTBB`

.. code-block:: bash

    sudo apt-get update
    auso apt-get install libtbb-dev

To install :program:`Conda`, please check this instruction:

.. toctree::
    :maxdepth: 1

    install_conda

To install :program:`CMake`, :program:`conan` and :program:`nodejs` with :program:`conda`:

.. code-block:: bash

    conda install -c conda-forge cmake nodejs conan

The rest of prerequisites, namely gcc and clang compilers, have to be installed from their source. Please check the following links for their installation:

- `A Youtube video to install gcc <https://www.youtube.com/watch?v=PhGbuI4iTno&t=173s>`_
- `A Youtube video to install clang <https://www.youtube.com/watch?v=LbhGPiGI_Qs&t=854s>`_

*******************
 Build the project
*******************

Step 1: Clone the latest version
================================

.. code-block:: bash

    git clone -b [latest version] https://github.com/YanzhaoW/srs-control.git
    cd srs-control
    git submodule update --init

Step 2: Build the source
========================

.. code-block:: bash

    cmake --workflow --preset default

or

.. code-block:: bash

    cmake --preset default -DCMAKE_INSTALL_PREFIX=[/usr/local] [optional settings]
    cmake --build ./build --target install -- -j[nproc]

Speed up the building process
-----------------------------

The building process could take a long time at the first time because the package manager (conan) needs to build the dependencies from the source as well. Thus, if your system can provide these dependencies, it may be faster to first install the pre-built binaries from your system package manager:

.. code-block:: bash

    dnf install boost-devel protobuf-devel protobuf-lite-devel protobuf-compiler

Then enable the ``USE_SYSTEM_LIBRARIES`` with CMake:

.. code-block:: bash

    cmake --preset default -DCMAKE_INSTALL_PREFIX=[/usr/local] -DUSE_SYSTEM_LIBRARIES=ON
    cmake --build ./build --target install -- -j[nproc]

Please check this page about the "optional settings":

.. toctree::
    :maxdepth: 1

    stubs/cmake_doc
