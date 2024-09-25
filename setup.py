from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "libndtp",
        ["ndtp.cpp"],
    ),
]

setup(
    name="libndtp",
    version="0.0.1",
    author="Your Name",
    author_email="your.email@example.com",
    description="A simple NDTP library",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.6",
)
