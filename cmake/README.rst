############################
CMake configuration option
############################

Synopsis
##########

.. code-block:: bash

  cmake --preset default -D[options]

Description
##############

The program uses CMake as the build system and provide additional options to configure the build options, such as whether to use the ROOT dependency, whether to build documentation and the location of the installation.

Options
#############

.. option:: USE_ROOT

  - ``OFF`` (default): The program would only compile with ROOT if ROOT exists.

  - ``ON``: CMake configuration will fail if ``ROOT`` is not found. 

.. option:: NO_ROOT

  - ``OFF`` (default): Same as ``-DUSE_ROOT=OFF``.

  - ``ON``: The program does NOT compiler with ROOT even if ROOT exists.

.. option:: BUILD_STATIC

   - ``OFF`` (default): Use the dynamic linkage of the standard C++ library, i.e. ``libstdc++.so``.

   - ``ON``: Use the static linkage of the standard C++ library, i.e. ``libstdc++.a``. If your system doesn't have ``libstdc++.a``, enabling this option would cause a failure of the build process.

.. option:: BUILD_TEST

   - ``OFF``: Disable the build the test scripts.

   - ``ON`` (default): Enable the build of the test scripts.

.. option:: BUILD_DOC

   - ``OFF`` (default): Disable the documentation building of the project.

   - ``ON``: Enable the build of the documentation as well (if ``BUILD_ONLY_DOC`` is ``OFF``).

.. option:: BUILD_ONLY_DOC

   - ``OFF`` (default) Build the whole project.

   - ``ON``: Only build the documentation for this project.

.. option:: CMAKE_INSTALL_PREFIX

   Specify the location of the installation. The default value is ``/usr/local``.

Example
#############

To build and install the project at ``/home/usr_name/software/srs-control`` without ROOT dependency:

.. code-block:: bash

  cmake --preset default . -DNO_ROOT=TRUE \
        -DCMAKE_INSTALL_PREFIX="/home/user_name/software/srs-control"

