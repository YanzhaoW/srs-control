# pylint: disable-all

import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

# from conan.tools.cmake import cmake_layout

BOOST_LIBS = (
    "atomic",
    "charconv",
    "chrono",
    "cobalt",
    "container",
    "context",
    "contract",
    "coroutine",
    "date_time",
    "exception",
    "fiber",
    "filesystem",
    "graph",
    "graph_parallel",
    "iostreams",
    "json",
    "locale",
    "log",
    "math",
    "mpi",
    "nowide",
    "process",
    "program_options",
    "python",
    "random",
    "regex",
    "serialization",
    "stacktrace",
    "system",
    "test",
    "thread",
    "timer",
    "type_erasure",
    "url",
    "wave",
)

BOOST_OPTIONS = {
    f"without_{_name}": True
    for _name in BOOST_LIBS
    if _name
    not in [
        "thread",
        "atomic",
        "chrono",
        "container",
        "date_time",
        "exception",
        "system",
    ]
}
BOOST_OPTIONS.update({"shared": False})

PROTOBUF_OPTIONS = {
    "with_zlib": True,
    "with_rtti": False,
    "fPIC": True,
    "shared": False,
    "lite": False,
}


class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("gsl-lite/0.41.0")  # type: ignore
        self.requires("cli11/2.4.2")  # type: ignore
        self.requires("spdlog/1.15.1")  # type: ignore
        self.requires("zpp_bits/4.4.24")  # type: ignore
        self.requires("magic_enum/0.9.7")  # type: ignore
        self.requires("fmt/12.1.0", override=True)  # type: ignore
        self.requires("taskflow/3.10.0")  # type: ignore
        self.requires("glaze/6.0.1")  # type: ignore

        if os.environ["CMAKE_USE_SYSTEM_BOOST"] == "OFF":
            print("---- Conan: compiling protobuf from the conan package manager.")
            self.requires("boost/1.88.0", options=BOOST_OPTIONS)  # type: ignore
        else:
            print("---- Conan: using Boost from the local system.")

        if os.environ["CMAKE_ENABLE_TEST"] == "ON":
            self.requires("gtest/1.15.0")  # type: ignore

        if os.environ["CMAKE_USE_SYSTEM_PROTOBUF"] == "OFF":
            print(
                "---- Conan: compiling protobuf from the conan package manager."
            )
            self.requires("protobuf/6.32.1", options=PROTOBUF_OPTIONS)  # type: ignore
        else:
            print("---- Conan: using protobuf from the local system.")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = ""
        tc.generate()

    # def layout(self):
    #     cmake_layout(self)

    def build_requirements(self):
        if os.environ["CMAKE_USE_SYSTEM_PROTOBUF"] == "OFF":
            self.tool_requires("protobuf/6.32.1")  # type: ignore
