from skbuild import setup
import setuptools

setup(
    name="libndtp",
    version="0.1.0",
    author="Your Name",
    author_email="your.email@example.com",
    description="A C++ extension for NDTP with Python bindings",
    long_description=open("README.md", encoding="utf-8").read(),
    long_description_content_type="text/markdown",
    packages=setuptools.find_packages(),
    include_package_data=True,  # Include package data as specified in MANIFEST.in
    cmake_minimum_required_version="3.20",
    cmake_source_dir=".",  # Ensure CMakeLists.txt is in the root of libndtp
    cmake_args=[
        "-DBUILD_TESTING=OFF",
        "-Dprotobuf_BUILD_TESTS=OFF",
        "-Dprotobuf_WITH_ZLIB=OFF",
        "-Dprotobuf_BUILD_EXAMPLES=OFF",
        "-Dprotobuf_BUILD_PROTOC_BINARIES=ON",  # Ensure protoc is built
        "-DCMAKE_BUILD_TYPE=Release",
        # Add any other necessary CMake arguments here
    ],
    python_requires=">=3.7",
    install_requires=[
        "pybind11>=2.6.0",
        "protobuf>=3.0.0",
        "grpcio-tools>=1.38.0",
        # Remove or correct any unnecessary dependencies
    ],
)
