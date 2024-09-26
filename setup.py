from setuptools import setup, Extension, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext
import os
import subprocess


def pkg_config(package, kw):
    flag_map = {"-I": "include_dirs", "-L": "library_dirs", "-l": "libraries"}
    try:
        tokens = (
            subprocess.check_output(["pkg-config", "--libs", "--cflags", package])
            .decode("utf-8")
            .split()
        )
    except subprocess.CalledProcessError:
        return kw

    for token in tokens:
        if token[:2] in flag_map:
            kw.setdefault(flag_map.get(token[:2]), []).append(token[2:])
        else:
            kw.setdefault("extra_compile_args", []).append(token)
    return kw


base_dir = os.path.dirname(os.path.abspath(__file__))
synapse_include_dir = os.path.join(base_dir, "synapse")

ext_kwargs = {
    "include_dirs": [synapse_include_dir],
    "extra_compile_args": ["-std=c++17"],
}

ext_kwargs = pkg_config("protobuf", ext_kwargs)

ext_modules = [
    Pybind11Extension("libndtp", ["ndtp.cpp"], **ext_kwargs),
]

setup(
    name="libndtp",
    version="0.1.0",
    author="Emma Zhou",
    author_email="emmaz@science.xyz",
    description="",
    long_description="",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.7",
    packages=find_packages(),
    package_data={
        "synapse": ["api/*.pb.h", "api/*.pb.cc"],
    },
    include_package_data=True,
)
