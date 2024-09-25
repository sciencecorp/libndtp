#include <pybind11/pybind11.h>

int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(libndtp, m) {
    m.doc() = "Simple NDTP library"; // optional module docstring
    m.def("add", &add, "A function that adds two numbers");
}