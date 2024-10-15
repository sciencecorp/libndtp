// bindings.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "libndtp/ndtp.h"
#include "libndtp/types.h"

namespace py = pybind11;
using namespace science::libndtp;

PYBIND11_MODULE(libndtp, m) {
    m.doc() = "libndtp C++ extension for NDTP with Python bindings";

    // Bind NDTPHeader
    py::class_<NDTPHeader>(m, "NDTPHeader")
        .def(py::init<>())
        .def_readwrite("version", &NDTPHeader::version)
        .def_readwrite("data_type", &NDTPHeader::data_type)
        .def_readwrite("timestamp", &NDTPHeader::timestamp)
        .def_readwrite("seq_number", &NDTPHeader::seq_number)
        .def("pack", &NDTPHeader::pack)
        .def_static("unpack", &NDTPHeader::unpack, py::arg("data"));

    // Bind NDTPPayloadBroadband::ChannelData
    py::class_<NDTPPayloadBroadband::ChannelData>(m, "ChannelData")
        .def(py::init<>())
        .def_readwrite("channel_id", &NDTPPayloadBroadband::ChannelData::channel_id)
        .def_readwrite("channel_data", &NDTPPayloadBroadband::ChannelData::channel_data)
        .def("pack", &NDTPPayloadBroadband::ChannelData::pack, py::arg("bit_width"), py::arg("signed_val"), py::arg("offset"), py::arg("payload"))
        .def_static("unpack", &NDTPPayloadBroadband::ChannelData::unpack, py::arg("data"), py::arg("bit_width"), py::arg("signed_val"), py::arg("offset"));

    // Bind NDTPPayloadBroadband
    py::class_<NDTPPayloadBroadband>(m, "NDTPPayloadBroadband")
        .def(py::init<>())
        .def_readwrite("signed_val", &NDTPPayloadBroadband::signed_val)
        .def_readwrite("bit_width", &NDTPPayloadBroadband::bit_width)
        .def_readwrite("sample_rate", &NDTPPayloadBroadband::sample_rate)
        .def_readwrite("channels", &NDTPPayloadBroadband::channels)
        .def("pack", &NDTPPayloadBroadband::pack)
        .def_static("unpack", &NDTPPayloadBroadband::unpack, py::arg("data"), py::arg("offset"));

    // Bind NDTPPayloadSpiketrain
    py::class_<NDTPPayloadSpiketrain>(m, "NDTPPayloadSpiketrain")
        .def(py::init<>())
        .def_readwrite("spike_counts", &NDTPPayloadSpiketrain::spike_counts)
        .def("pack", &NDTPPayloadSpiketrain::pack)
        .def_static("unpack", &NDTPPayloadSpiketrain::unpack, py::arg("data"), py::arg("offset"));

    // Bind NDTPMessage
    py::class_<NDTPMessage>(m, "NDTPMessage")
        .def(py::init<>())
        .def_readwrite("header", &NDTPMessage::header)
        .def_readwrite("broadband_payload", &NDTPMessage::broadband_payload)
        .def_readwrite("spiketrain_payload", &NDTPMessage::spiketrain_payload)
        .def("pack", &NDTPMessage::pack)
        .def_static("unpack", &NDTPMessage::unpack, py::arg("data"));

    // Bind ElectricalBroadbandData
    py::class_<ElectricalBroadbandData>(m, "ElectricalBroadbandData")
        .def(py::init<>())
        .def_readwrite("bit_width", &ElectricalBroadbandData::bit_width)
        .def_readwrite("signed_val", &ElectricalBroadbandData::signed_val)
        .def_readwrite("sample_rate", &ElectricalBroadbandData::sample_rate)
        .def_readwrite("t0", &ElectricalBroadbandData::t0)
        .def_readwrite("channels", &ElectricalBroadbandData::channels)
        .def("pack", &ElectricalBroadbandData::pack, py::arg("seq_number"))
        .def_static("unpack", &ElectricalBroadbandData::unpack, py::arg("msg"));

    // Bind BinnedSpiketrainData
    py::class_<BinnedSpiketrainData>(m, "BinnedSpiketrainData")
        .def(py::init<>())
        .def_readwrite("spike_counts", &BinnedSpiketrainData::spike_counts)
        .def("pack", &BinnedSpiketrainData::pack, py::arg("seq_number"))
        .def_static("unpack", &BinnedSpiketrainData::unpack, py::arg("msg"));

    // Bind SynapseData using std::variant
    // Currently, Pybind11 doesn't support std::variant directly.
    // Alternative approaches can be implemented as needed.
}