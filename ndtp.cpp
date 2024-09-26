#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <cstdint>
#include <variant>

#include <google/protobuf/port_def.inc>
#include <google/protobuf/port_undef.inc>
#include "synapse/api/datatype.pb.h"

namespace py = pybind11;
using synapse::DataType;

class NDTPPayloadBroadband;
class NDTPPayloadSpiketrain;
class NDTPHeader;
class NDTPMessage;

const uint8_t NDTP_VERSION = 1;

class NDTPPayloadBroadband {
public:
    struct ChannelData {
        int channel_id;
        std::vector<int> channel_data;
    };

    bool signed_;
    int bit_width;
    int sample_rate;
    std::vector<ChannelData> channels;

    std::vector<uint8_t> pack() const;
    static NDTPPayloadBroadband unpack(const std::vector<uint8_t>& data);
};

class NDTPPayloadSpiketrain {
public:
    static const int BIT_WIDTH = 2;
    std::vector<int> spike_counts;

    std::vector<uint8_t> pack() const;
    static NDTPPayloadSpiketrain unpack(const std::vector<uint8_t>& data);
};

class NDTPHeader {
public:
    int data_type;
    uint64_t timestamp;
    uint16_t seq_number;

    std::vector<uint8_t> pack() const;
    static NDTPHeader unpack(const std::vector<uint8_t>& data);
};

class NDTPMessage {
public:
    NDTPHeader header;
    std::variant<NDTPPayloadBroadband, NDTPPayloadSpiketrain> payload;

    static uint16_t crc16(const std::vector<uint8_t>& data, uint16_t poly = 0x8005, uint16_t init = 0xFFFF);
    static bool crc16_verify(const std::vector<uint8_t>& data, uint16_t crc16);
    std::vector<uint8_t> pack() const;
    static NDTPMessage unpack(const std::vector<uint8_t>& data);
};

std::string convert_to_hex(const std::string& byte_string) {
    std::ostringstream result;
    for (unsigned char c : byte_string) {
        result << "\\x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(c);
    }
    return result.str();
}

std::string bytes_to_binary(const std::vector<uint8_t>& data, int start_bit) {
    std::string binary_string;
    for (uint8_t byte : data) {
        binary_string += std::bitset<8>(byte).to_string();
    }
    binary_string = binary_string.substr(start_bit);
    std::string result;
    for (size_t i = 0; i < binary_string.length(); i += 8) {
        if (i > 0) result += " ";
        result += binary_string.substr(i, 8);
    }
    return result;
}

std::tuple<std::vector<uint8_t>, int> to_bytes(
    const std::vector<int>& values,
    int bit_width,
    const std::vector<uint8_t>& existing = std::vector<uint8_t>(),
    int writing_bit_offset = 0,
    bool signed_ = false
) {
    if (bit_width <= 0) {
        throw std::invalid_argument("bit width must be > 0");
    }

    int truncate_bytes = writing_bit_offset / 8;
    writing_bit_offset %= 8;

    std::vector<uint8_t> result(existing.begin() + truncate_bytes, existing.end());
    bool continue_last = !existing.empty() && writing_bit_offset > 0;
    uint8_t current_byte = continue_last ? result.back() : 0;
    int bits_in_current_byte = writing_bit_offset;

    for (int value : values) {
        if (signed_) {
            int min_value = -(1 << (bit_width - 1));
            int max_value = (1 << (bit_width - 1)) - 1;
            if (value < min_value || value > max_value) {
                throw std::invalid_argument("signed value doesn't fit in specified bits");
            }
            if (value < 0) {
                value = (1 << bit_width) + value;
            }
        } else {
            if (value < 0) {
                throw std::invalid_argument("unsigned packing specified, but value is negative");
            }
            if (value >= (1 << bit_width)) {
                throw std::invalid_argument("unsigned value doesn't fit in specified bits");
            }
        }

        int remaining_bits = bit_width;
        while (remaining_bits > 0) {
            int available_bits = 8 - bits_in_current_byte;
            int bits_to_write = std::min(available_bits, remaining_bits);

            int shift = remaining_bits - bits_to_write;
            int bits_to_add = (value >> shift) & ((1 << bits_to_write) - 1);

            current_byte |= bits_to_add << (available_bits - bits_to_write);

            remaining_bits -= bits_to_write;
            bits_in_current_byte += bits_to_write;

            if (bits_in_current_byte == 8) {
                if (continue_last) {
                    result.back() = current_byte;
                    continue_last = false;
                } else {
                    result.push_back(current_byte);
                }
                current_byte = 0;
                bits_in_current_byte = 0;
            }
        }
    }

    if (bits_in_current_byte > 0) {
        result.push_back(current_byte);
    }

    return std::make_tuple(result, bits_in_current_byte);
}

std::tuple<std::vector<int>, int, std::vector<uint8_t>> to_ints(
    const std::vector<uint8_t>& data,
    int bit_width,
    int count = 0,
    int start_bit = 0,
    bool signed_ = false
) {
    if (bit_width <= 0) {
        throw std::invalid_argument("bit width must be > 0");
    }

    int truncate_bytes = start_bit / 8;
    start_bit %= 8;

    std::vector<uint8_t> truncated_data(data.begin() + truncate_bytes, data.end());

    if (count > 0 && truncated_data.size() < std::ceil(bit_width * count / 8.0)) {
        throw std::invalid_argument("insufficient data for requested number of values");
    }

    std::vector<int> values;
    int current_value = 0;
    int bits_in_current_value = 0;
    int mask = (1 << bit_width) - 1;
    int total_bits_read = 0;

    for (size_t byte_index = 0; byte_index < truncated_data.size(); ++byte_index) {
        uint8_t byte = truncated_data[byte_index];
        int start = (byte_index == 0) ? start_bit : 0;
        for (int bit_index = 7 - start; bit_index >= 0; --bit_index) {
            int bit = (byte >> bit_index) & 1;
            current_value = (current_value << 1) | bit;
            bits_in_current_value++;
            total_bits_read++;

            if (bits_in_current_value == bit_width) {
                if (signed_ && (current_value & (1 << (bit_width - 1)))) {
                    current_value = current_value - (1 << bit_width);
                } else {
                    current_value &= mask;
                }
                values.push_back(current_value);
                current_value = 0;
                bits_in_current_value = 0;
            }

            if (count > 0 && static_cast<int>(values.size()) == count) {
                int end_bit = start_bit + total_bits_read;
                return std::make_tuple(values, end_bit, truncated_data);
            }
        }
    }

    if (bits_in_current_value > 0) {
        if (bits_in_current_value == bit_width) {
            if (signed_ && (current_value & (1 << (bit_width - 1)))) {
                current_value = current_value - (1 << bit_width);
            } else {
                current_value &= mask;
            }
            values.push_back(current_value);
        } else if (count == 0) {
            throw std::runtime_error("bits left over, not enough to form a complete value");
        }
    }

    if (count > 0) {
        values.resize(std::min(static_cast<size_t>(count), values.size()));
    }

    int end_bit = start_bit + total_bits_read;
    return std::make_tuple(values, end_bit, truncated_data);
}

std::vector<uint8_t> NDTPPayloadBroadband::pack() const {
    std::vector<uint8_t> payload;
    
    // Pack signed and bit_width
    payload.push_back((bit_width & 0x7F) << 1 | (signed_ ? 1 : 0));

    // Pack number of channels
    int n_channels = channels.size();
    payload.push_back((n_channels >> 16) & 0xFF);
    payload.push_back((n_channels >> 8) & 0xFF);
    payload.push_back(n_channels & 0xFF);

    // Pack sample rate
    payload.push_back((sample_rate >> 8) & 0xFF);
    payload.push_back(sample_rate & 0xFF);

    // Pack channel data
    int b_offset = 0;
    for (const auto& channel : channels) {
        std::vector<uint8_t> channel_payload;
        std::tie(channel_payload, b_offset) = to_bytes({channel.channel_id}, 24, payload, b_offset, false);
        payload = channel_payload;

        std::tie(channel_payload, b_offset) = to_bytes({static_cast<int>(channel.channel_data.size())}, 16, payload, b_offset, false);
        payload = channel_payload;

        std::tie(channel_payload, b_offset) = to_bytes(channel.channel_data, bit_width, payload, b_offset, signed_);
        payload = channel_payload;
    }

    return payload;
}

NDTPPayloadBroadband NDTPPayloadBroadband::unpack(const std::vector<uint8_t>& data) {
    if (data.size() < 6) {
        throw std::invalid_argument("Invalid broadband data size");
    }

    NDTPPayloadBroadband result;
    
    // Unpack signed and bit_width
    result.bit_width = data[0] >> 1;
    result.signed_ = (data[0] & 1) == 1;

    // Unpack number of channels
    int num_channels = (data[1] << 16) | (data[2] << 8) | data[3];

    // Unpack sample rate
    result.sample_rate = (data[4] << 8) | data[5];

    std::vector<uint8_t> payload(data.begin() + 6, data.end());
    int end_bit = 0;

    for (int c = 0; c < num_channels; ++c) {
        ChannelData channel;

        std::vector<int> channel_id;
        std::tie(channel_id, end_bit, payload) = to_ints(payload, 24, 1, end_bit, false);
        channel.channel_id = channel_id[0];

        std::vector<int> num_samples;
        std::tie(num_samples, end_bit, payload) = to_ints(payload, 16, 1, end_bit, false);

        std::tie(channel.channel_data, end_bit, payload) = to_ints(payload, result.bit_width, num_samples[0], end_bit, result.signed_);

        result.channels.push_back(channel);
    }

    return result;
}

std::vector<uint8_t> NDTPPayloadSpiketrain::pack() const {
    std::vector<uint8_t> payload;
    
    // Pack number of spike counts
    uint32_t count = spike_counts.size();
    payload.push_back((count >> 24) & 0xFF);
    payload.push_back((count >> 16) & 0xFF);
    payload.push_back((count >> 8) & 0xFF);
    payload.push_back(count & 0xFF);

    // Pack spike counts
    auto [packed_data, _] = to_bytes(spike_counts, BIT_WIDTH, payload, 0, false);
    return packed_data;
}

NDTPPayloadSpiketrain NDTPPayloadSpiketrain::unpack(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        throw std::invalid_argument("Invalid spiketrain data size");
    }

    uint32_t num_spikes = (static_cast<uint32_t>(data[0]) << 24) |
                          (static_cast<uint32_t>(data[1]) << 16) |
                          (static_cast<uint32_t>(data[2]) << 8) |
                          static_cast<uint32_t>(data[3]);

    std::vector<uint8_t> payload(data.begin() + 4, data.end());
    auto [unpacked, _, __] = to_ints(payload, BIT_WIDTH, num_spikes, 0, false);

    return NDTPPayloadSpiketrain{unpacked};
}

std::vector<uint8_t> NDTPHeader::pack() const {
    std::vector<uint8_t> result(12);
    result[0] = NDTP_VERSION;
    result[1] = static_cast<uint8_t>(data_type);
    std::memcpy(&result[2], &timestamp, sizeof(timestamp));
    std::memcpy(&result[10], &seq_number, sizeof(seq_number));
    return result;
}

NDTPHeader NDTPHeader::unpack(const std::vector<uint8_t>& data) {
    if (data.size() < 12) {
        throw std::invalid_argument("Invalid header size");
    }

    uint8_t version = data[0];
    if (version != NDTP_VERSION) {
        throw std::runtime_error("Incompatible version");
    }

    int data_type = data[1];
    uint64_t timestamp;
    uint16_t seq_number;

    std::memcpy(&timestamp, &data[2], sizeof(timestamp));
    std::memcpy(&seq_number, &data[10], sizeof(seq_number));

    return NDTPHeader{data_type, timestamp, seq_number};
}

uint16_t NDTPMessage::crc16(const std::vector<uint8_t>& data, uint16_t poly, uint16_t init) {
    uint16_t crc = init;

    for (uint8_t byte : data) {
        crc ^= static_cast<uint16_t>(byte) << 8;
        for (int _ = 0; _ < 8; ++_) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc & 0xFFFF;
}

bool NDTPMessage::crc16_verify(const std::vector<uint8_t>& data, uint16_t crc16) {
    return NDTPMessage::crc16(data) == crc16;
}

std::vector<uint8_t> NDTPMessage::pack() const {
    std::vector<uint8_t> result = header.pack();
    
    std::vector<uint8_t> payload_data;
    std::visit([&payload_data](const auto& payload) {
        payload_data = payload.pack();
    }, payload);
    
    result.insert(result.end(), payload_data.begin(), payload_data.end());

    uint16_t crc = crc16(result);
    result.push_back((crc >> 8) & 0xFF);
    result.push_back(crc & 0xFF);

    return result;
}

NDTPMessage NDTPMessage::unpack(const std::vector<uint8_t>& data) {
    if (data.size() < 14) {  // Minimum size: 12 (header) + 2 (CRC)
        throw std::invalid_argument("Invalid message size");
    }

    std::vector<uint8_t> header_data(data.begin(), data.begin() + 12);
    NDTPHeader header = NDTPHeader::unpack(header_data);

    uint16_t crc = (static_cast<uint16_t>(data[data.size() - 2]) << 8) |
                   static_cast<uint16_t>(data[data.size() - 1]);

    if (!crc16_verify(std::vector<uint8_t>(data.begin(), data.end() - 2), crc)) {
        throw std::runtime_error("CRC16 verification failed");
    }

    std::vector<uint8_t> payload_data(data.begin() + 12, data.end() - 2);

    std::variant<NDTPPayloadBroadband, NDTPPayloadSpiketrain> payload;
    if (header.data_type == static_cast<int>(DataType::kBroadband)) {
        payload = NDTPPayloadBroadband::unpack(payload_data);
    } else if (header.data_type == static_cast<int>(DataType::kSpiketrain)) {
        payload = NDTPPayloadSpiketrain::unpack(payload_data);
    } else {
        throw std::runtime_error("Unknown data type");
    }

    return NDTPMessage{header, payload};
}

class ElectricalBroadbandData {
public:
    struct ChannelData {
        int channel_id;
        std::vector<int> channel_data;
    };

    static const DataType data_type = DataType::kBroadband;

    int bit_width;
    bool signed_;
    int sample_rate;
    int64_t t0;
    std::vector<ChannelData> channels;

    std::vector<std::vector<uint8_t>> pack(int seq_number) const {
        std::vector<std::vector<uint8_t>> packets;
        int seq_number_offset = 0;

        for (const auto& c : channels) {
            NDTPMessage msg;
            msg.header.data_type = DataType::kBroadband;
            msg.header.timestamp = t0;
            msg.header.seq_number = seq_number + seq_number_offset;

            NDTPPayloadBroadband payload;
            payload.bit_width = bit_width;
            payload.signed_ = signed_;
            payload.sample_rate = sample_rate;
            payload.channels.push_back({c.channel_id, c.channel_data});

            msg.payload = payload;
            packets.push_back(msg.pack());
            seq_number_offset++;
        }

        return packets;
    }

    static ElectricalBroadbandData from_ndtp_message(const NDTPMessage& msg) {
        ElectricalBroadbandData result;
        result.t0 = msg.header.timestamp;
        const auto& payload = std::get<NDTPPayloadBroadband>(msg.payload);
        result.bit_width = payload.bit_width;
        result.signed_ = payload.signed_;
        result.sample_rate = payload.sample_rate;

        for (const auto& c : payload.channels) {
            result.channels.push_back({c.channel_id, c.channel_data});
        }

        return result;
    }

    static ElectricalBroadbandData unpack(const std::vector<uint8_t>& data) {
        NDTPMessage msg = NDTPMessage::unpack(data);
        return from_ndtp_message(msg);
    }
};

class SpiketrainData {
public:
    static const DataType data_type = DataType::kSpiketrain;

    int64_t t0;
    std::vector<int> spike_counts;

    std::vector<std::vector<uint8_t>> pack(int seq_number) const {
        NDTPMessage msg;
        msg.header.data_type = DataType::kSpiketrain;
        msg.header.timestamp = t0;
        msg.header.seq_number = seq_number;

        NDTPPayloadSpiketrain payload;
        payload.spike_counts = spike_counts;

        msg.payload = payload;
        return {msg.pack()};
    }

    static SpiketrainData from_ndtp_message(const NDTPMessage& msg) {
        SpiketrainData result;
        result.t0 = msg.header.timestamp;
        const auto& payload = std::get<NDTPPayloadSpiketrain>(msg.payload);
        result.spike_counts = payload.spike_counts;
        return result;
    }

    static SpiketrainData unpack(const std::vector<uint8_t>& data) {
        NDTPMessage msg = NDTPMessage::unpack(data);
        return from_ndtp_message(msg);
    }
};


PYBIND11_MODULE(libndtp, m) {
    py::class_<NDTPHeader>(m, "NDTPHeader")
        .def(py::init<>())
        .def_readwrite("data_type", &NDTPHeader::data_type)
        .def_readwrite("timestamp", &NDTPHeader::timestamp)
        .def_readwrite("seq_number", &NDTPHeader::seq_number);

    py::class_<NDTPPayloadBroadband::ChannelData>(m, "NDTPPayloadBroadbandChannelData")
        .def(py::init<>())
        .def_readwrite("channel_id", &NDTPPayloadBroadband::ChannelData::channel_id)
        .def_readwrite("channel_data", &NDTPPayloadBroadband::ChannelData::channel_data);

    py::class_<NDTPPayloadBroadband>(m, "NDTPPayloadBroadband")
        .def(py::init<>())
        .def_readwrite("bit_width", &NDTPPayloadBroadband::bit_width)
        .def_readwrite("signed_", &NDTPPayloadBroadband::signed_)
        .def_readwrite("sample_rate", &NDTPPayloadBroadband::sample_rate)
        .def_readwrite("channels", &NDTPPayloadBroadband::channels);

    py::class_<NDTPPayloadSpiketrain>(m, "NDTPPayloadSpiketrain")
        .def(py::init<>())
        .def_readwrite("spike_counts", &NDTPPayloadSpiketrain::spike_counts);

    py::class_<NDTPMessage>(m, "NDTPMessage")
        .def(py::init<>())
        .def_readwrite("header", &NDTPMessage::header)
        .def_readwrite("payload", &NDTPMessage::payload)
        .def("pack", &NDTPMessage::pack)
        .def_static("unpack", &NDTPMessage::unpack);

    py::class_<ElectricalBroadbandData>(m, "ElectricalBroadbandData")
        .def(py::init<>())
        .def_readwrite("bit_width", &ElectricalBroadbandData::bit_width)
        .def_readwrite("signed_", &ElectricalBroadbandData::signed_)
        .def_readwrite("sample_rate", &ElectricalBroadbandData::sample_rate)
        .def_readwrite("t0", &ElectricalBroadbandData::t0)
        .def_readwrite("channels", &ElectricalBroadbandData::channels)
        .def("pack", &ElectricalBroadbandData::pack)
        .def_static("from_ndtp_message", &ElectricalBroadbandData::from_ndtp_message)
        .def_static("unpack", &ElectricalBroadbandData::unpack)
        .def_readonly_static("data_type", &ElectricalBroadbandData::data_type);

    py::class_<ElectricalBroadbandData::ChannelData>(m, "ElectricalBroadbandChannelData")
        .def(py::init<>())
        .def_readwrite("channel_id", &ElectricalBroadbandData::ChannelData::channel_id)
        .def_readwrite("channel_data", &ElectricalBroadbandData::ChannelData::channel_data);

    py::class_<SpiketrainData>(m, "SpiketrainData")
        .def(py::init<>())
        .def_readwrite("t0", &SpiketrainData::t0)
        .def_readwrite("spike_counts", &SpiketrainData::spike_counts)
        .def("pack", &SpiketrainData::pack)
        .def_static("from_ndtp_message", &SpiketrainData::from_ndtp_message)
        .def_static("unpack", &SpiketrainData::unpack)
        .def_readonly_static("data_type", &SpiketrainData::data_type);
}