######################
 Download the project
######################

.. contents:: Contents
    :depth: 99

*************************
 System package managers
*************************

.. note::

    The program installed with system package managers doesn't have support of any ROOT-related features.

Fedora
======

Register rpm repository:

.. code-block:: bash

    sudo cat > /etc/yum.repos.d/srs-control.repo << EOF
    [srs-control]
    name=SRS-control: A data acquisition program for SRS FEC & VMM3
    baseurl=https://web-docs.gsi.de/~yanwang/repos/srs-control/rpm/fedora/$releasever/$basearch
    enabled=1
    gpgcheck=1
    gpgkey=https://web-docs.gsi.de/~yanwang/repos/srs-control/public-key.asc
    metadata_expire=10m
    EOF

To install the software:

.. code-block:: bash

    sudo dnf install srs-control

To upgrade the software to the latest release:

.. code-block:: bash

    sudo dnf upgrade srs-control

Debian
======

bullseye (Debian 11)
--------------------

Add the GPG key and repo registry (only once):

.. code-block:: bash

    sudo wget -O /etc/apt/keyrings/srs-control.asc \
    https://web-docs.gsi.de/~yanwang/repos/srs-control/public-key.asc

    sudo echo "deb [signed-by=/etc/apt/keyrings/srs-control.asc] \
    https://web-docs.gsi.de/~yanwang/repos/srs-control/apt/bullseye bullseye main" \
    | sudo tee /etc/apt/sources.list.d/srs-control.list

    sudo apt update -y

To install the package:

.. code-block:: bash

    sudo apt install srs-control

To upgrade the software to the latest release:

.. code-block:: bash

    sudo apt update
    sudo apt install srs-control

Ubuntu
======

jammy (Ubuntu 22)
-----------------

Add the GPG key and repo registry (only once):

.. code-block:: bash

    sudo wget -O /etc/apt/keyrings/srs-control.asc \
    https://web-docs.gsi.de/~yanwang/repos/srs-control/public-key.asc

    sudo echo "deb [signed-by=/etc/apt/keyrings/srs-control.asc] \
    https://web-docs.gsi.de/~yanwang/repos/srs-control/apt/jammy jammy main" \
    | sudo tee /etc/apt/sources.list.d/srs-control.list

    sudo apt update -y

To install the package:

.. code-block:: bash

    sudo apt install srs-control

To upgrade the software to the latest release:

.. code-block:: bash

    sudo apt update
    sudo apt install srs-control

********************
 Pre-built binaries
********************

Please visit the `release page <https://github.com/YanzhaoW/srs-control/releases>`_ and download the latest release for your operating system:

.. code-block:: bash

    wget [download-link]
    tar -xvzf [download-file]

After unzipping the downloaded file, a new folder ``srs-download`` will be put in the current folder.

If your operating system is not in the download link list. Please either :doc:`install the project from source </installation>` or create an issue to make the request.

.. important::

    If the ROOT support is needed, please install the same ROOT version used in the download link.
