# SRS-control - A data acquisition program for SRS FEC & VMM3

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7e8c956af1bc46c7836524f1ace32c11)](https://app.codacy.com/gh/YanzhaoW/srs-control/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![CI pipeline](https://github.com/YanzhaoW/srs-control/actions/workflows/ci.yml/badge.svg?branch=dev)](https://github.com/YanzhaoW/srs-control/actions?query=branch%3Adev)
[![Github Releases](https://img.shields.io/github/release/YanzhaoW/srs-control.svg)](https://github.com/YanzhaoW/srs-control/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

SRS-control is a program containing a command line tool `srs_control` and additional library APIs that can be used to communicate with SRS system and interpret the received data in your own program.

Please check the full documentation of this project: [srs-contorl documentation](<https://yanzhaow.github.io/srs-control/>).

Reference of the internal code implementation can be checked from this [Doxygen documentation](https://apps.ikp.uni-koeln.de/~ywang/srs/).

## `srs_control` preview

![Imgur](doc/media/srs_control_preview_v1.gif)

## Acknowledgments

- A lot of information was used from the existing codebase of the VMM slow control software [vmmsc](https://gitlab.cern.ch/rd51-slow-control/vmmsc.git).
- The communication protocols to the SRS system were adopted from [srslib](https://github.com/bl0x/srslib).
