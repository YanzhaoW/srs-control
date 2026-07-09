# pylint: disable-all

import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

# from conan.tools.cmake import cmake_layout

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
        self.requires("fmt/12.1.0", force=True, options={"header_only": True})  # type: ignore
        self.requires("spdlog/1.17.0")  # type: ignore
        self.requires("zpp_bits/4.4.24")  # type: ignore
        self.requires("magic_enum/0.9.7")  # type: ignore
        self.requires("taskflow/3.10.0")  # type: ignore
        self.requires("glaze/6.0.1")  # type: ignore
        self.requires("concurrentqueue/1.0.5")  # type: ignore
        self.requires("asio/1.38.0")  # type: ignore

        if os.environ["CMAKE_USE_SYSTEM_RE2"] == "OFF":
            print("---- Conan: compiling RE2 from the conan package manager.")
            self.requires("re2/20251105")  # type: ignore
        else:
            print("---- Conan: using RE2 from the local system.")

        if os.environ["CMAKE_ENABLE_TEST"] == "ON":
            self.requires("gtest/1.15.0")  # type: ignore

        if os.environ["CMAKE_USE_SYSTEM_PROTOBUF"] == "OFF":
            print(
                "---- Conan: compiling protobuf from the conan package manager."
            )
            self.requires("protobuf/7.35.0", options=PROTOBUF_OPTIONS)  # type: ignore
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
