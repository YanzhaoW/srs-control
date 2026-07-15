# SRS-control - Data acquisition for SRS FEC & VMM3

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7e8c956af1bc46c7836524f1ace32c11)](https://app.codacy.com/gh/YanzhaoW/srs-control/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![codecov](https://codecov.io/gh/YanzhaoW/srs-control/graph/badge.svg?token=4G5AUUZ0LU)](https://codecov.io/gh/YanzhaoW/srs-control)
[![CI pipeline](https://github.com/YanzhaoW/srs-control/actions/workflows/ci.yml/badge.svg?branch=dev)](https://github.com/YanzhaoW/srs-control/actions?query=branch%3Adev)
[![Github Releases](https://img.shields.io/github/release/YanzhaoW/srs-control.svg)](https://github.com/YanzhaoW/srs-control/releases)
[![dashboard](https://img.shields.io/badge/dashboard-srs-blue?labelColor=gray&style=flat)](https://my.cdash.org/index.php?project=srs-control)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.21346083.svg)](https://doi.org/10.5281/zenodo.21346083)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

SRS-control is a data acquisition program for SRS FEC and VMM3 systems. It provides the `srs-control` command-line tool and library APIs for communicating with SRS hardware and data processing in your own applications.

Please check the full documentation of this project: [srs-contorl documentation](https://yanzhaow.github.io/srs-control/).

Reference of the internal code implementation can be checked from this [Doxygen documentation](https://web-docs.gsi.de/~yanwang/srs).

## Installation

`srs-control` can be installed via system package managers in the following Linux distros:

- Debian: `bullseye`
- Ubuntu: `jammy`
- Fedora: `44`

See [Download the project](https://yanzhaow.github.io/srs-control/download_step.html) for detailed information.

## Synopsis

```bash
srs-control -h
```

```text
SRS system command line interface
Usage: ./srs-control [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -v,--version                Show the current version
  --root-version              Show the ROOT version if used
  -a,--action ENUM [all]      Set the action of program
                              Available options: [all, acq-on, acq-off, read-only, none]
  -l,--log-level ENUM [info]  Set log level
                              Available options: [trace, debug, info, warn, err, critical, off, n-levels]
  -r,--run-time UINT [0]      Set the runtime of the application. Non-stop if 0
  -p,--print-mode ENUM [print-speed]
                              Set data print mode
                              Available options: [print-speed, print-header, print-raw, print-all]
  -c,--config-file TEXT [~/.config/srs-control/config.yaml]
                              Set the path of the JSON config file
  -s,--split-output INT [1]   Splitting the output data into different files.
  --dump-config TEXT [~/.config/srs-control/config.yaml]
                              Dump default configuration to the file
  -o,--output-files TEXT [[]]  ...
                              Set output file (or socket) names
```

See [Command line tools](https://yanzhaow.github.io/srs-control/executable.html#srs-control) for detailed information.

## Configuration file

Default configuration:

```yaml
# Local port used to communicate with FECs
fec_control_local_port: 6007

# Remote port used to communicate with FECs
fec_control_remote_port: 6600

# Local port numbers used to read data from FECs
fec_data_receive_ports:
  - 6006

# The size of the buffer in bytes
data_buffer_size: 80000

# Remote FEC IP addresses
remote_fec_ips:
  - "10.0.0.2"

# Buffer queue size
buffer_queue_capacity: 100

# Output filenames
output_filenames: []

# Number of Output splits
output_split: 1

# Time (milliseconds) to wait after turning off FECs
time_wait_after_acq_off_ms: 1000

# Enable thread ID display from the console outputs
show_thread_id: false

# Enabling warnings when data drop occur
warn_if_data_drop: false

# Print mode
data_print_mode: print_speed
```

See [Configuration](https://yanzhaow.github.io/srs-control/executable.html#configuration) for detailed information.

## Testing and emulators

Emulators for FEC devices can be run with

```bash
srs-fec-emualtor emulator.yaml
```

This will launch multiple emulated FEC devices according to the setup in the configuration file, waiting for connections from `srs-control`.

To receive the data from those emulators, simply run the `srs-cotrol`:

```bash
srs-control -c config.yaml
```

See [Command line tools](https://yanzhaow.github.io/srs-control/executable.html#srs-control) for detailed information.

## Acknowledgments

- A lot of information was used from the existing codebase of the VMM slow control software [vmmsc](https://gitlab.cern.ch/rd51-slow-control/vmmsc.git).
- The communication protocols to the SRS system were adopted from [srslib](https://github.com/bl0x/srslib).
