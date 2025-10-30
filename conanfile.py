# pylint: disable-all

# import os

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
        "cobalt",
        "container",
        "context",
    ]
}
BOOST_OPTIONS.update({"shared": False})

PROTOBUF_OPTIONS = {
    "with_zlib": True,
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
        self.requires("fmt/11.1.4", override=True)  # type: ignore
        self.requires("boost/1.88.0", options=BOOST_OPTIONS)  # type: ignore
        self.requires("protobuf/6.30.1", options=PROTOBUF_OPTIONS)  # type: ignore
        self.requires("gtest/1.15.0")  # type: ignore

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = ""
        tc.generate()

    # def layout(self):
    #     cmake_layout(self)

    def build_requirements(self):
        self.tool_requires("protobuf/6.30.1")  # type: ignore
