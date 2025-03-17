import os
from sys import platform

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

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
    if _name not in ["thread", "atomic", "chrono", "container", "date_time", "exception", "system"]
}
BOOST_OPTIONS.update({"shared": False})


class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("gsl-lite/0.41.0")
        self.requires("cli11/2.4.2")
        self.requires("spdlog/1.15.1")
        self.requires("zpp_bits/4.4.24")
        self.requires("fmt/11.1.4", override=True)
        self.requires("boost/1.86.0", options=BOOST_OPTIONS)
        if platform != "darwin":
            self.requires("protobuf/5.27.0", options={"with_zlib": True, "fPIC": True, "shared": False, "lite": False})
            if os.environ["CMAKE_ENABLE_TEST"] == "ON":
                self.requires("gtest/1.15.0")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False
        tc.generate()
