// src/Ndtp.cpp
#include "libndtp/Ndtp.hpp"
#include "libndtp/Utilities.hpp"
#include <iostream>

namespace libndtp {

// Implementation of NDTPHeader
ByteArray NDTPHeader::pack() const {
    ByteArray data;
    // First byte: version (7 bits) and signed flag (1 bit)
    uint8_t first_byte = (bit_width & 0x7F) << 1 | (signed_val ? 1 : 0);
    data.push_back(first_byte);

    // DataType as 24 bits
    data.push_back((data_type >> 16) & 0xFF);
    data.push_back((data_type >> 8) & 0xFF);
    data.push_back(data_type & 0xFF);

    // Sample rate as 16 bits
    uint16_t sample_rate = static_cast<uint16_t>(timestamp);
    data.push_back((sample_rate >> 8) & 0xFF);
    data.push_back(sample_rate & 0xFF);

    // Timestamp as 64 bits
    for(int i = 7; i >= 0; --i) {
        data.push_back((timestamp >> (i * 8)) & 0xFF);
    }

    // Sequence number as 16 bits
    data.push_back((seq_number >> 8) & 0xFF);
    data.push_back(seq_number & 0xFF);

    return data;
}

NDTPHeader NDTPHeader::unpack(const ByteArray& data) {
    if(data.size() < 14) { // 1 + 3 + 2 + 8 + 2 = 16, adjust as per actual fields
        throw std::invalid_argument("Invalid header size");
    }

    NDTPHeader header;
    // First byte
    header.version = data[0] >> 1;
    header.signed_val = data[0] & 1;
    header.bit_width = (data[0] >> 1) & 0x7F;

    // DataType
    header.data_type = (data[1] << 16) | (data[2] << 8) | data[3];

    // Sample rate
    header.sample_rate = (data[4] << 8) | data[5];

    // Timestamp
    header.timestamp = 0;
    for(int i = 6; i < 14; ++i) {
        header.timestamp = (header.timestamp << 8) | data[i];
    }

    // Sequence number
    header.seq_number = (data[14] << 8) | data[15];

    return header;
}

// Implementation of NDTPPayloadBroadband::ChannelData
void NDTPPayloadBroadband::ChannelData::pack(int bit_width, bool signed_val, BitOffset& offset, ByteArray& payload) const {
    // Pack channel_id (24 bits)
    std::vector<int> channel_id_vector = { static_cast<int>(channel_id) };
    auto [packed_channel_id, new_offset] = to_bytes(channel_id_vector, 24, payload, offset, false);
    payload = packed_channel_id;
    offset = new_offset;

    // Pack number of samples (16 bits)
    std::vector<int> num_samples_vector = { static_cast<int>(channel_data.size()) };
    auto [packed_num_samples, updated_offset] = to_bytes(num_samples_vector, 16, payload, offset, false);
    payload = packed_num_samples;
    offset = updated_offset;

    // Pack channel_data
    auto [packed_data, final_offset] = to_bytes(channel_data, bit_width, payload, offset, signed_val);
    payload = packed_data;
    offset = final_offset;
}

NDTPPayloadBroadband::ChannelData NDTPPayloadBroadband::ChannelData::unpack(const ByteArray& data, int bit_width, bool signed_val, BitOffset& offset) {
    NDTPPayloadBroadband::ChannelData cd;

    // Unpack channel_id (24 bits)
    auto [channel_id_values, new_offset, remaining_data] = to_ints(data, 24, 1, offset, false);
    cd.channel_id = static_cast<uint32_t>(channel_id_values[0]);
    offset = new_offset;

    // Unpack number of samples (16 bits)
    auto [num_samples_values, updated_offset, _] = to_ints(remaining_data, 16, 1, offset, false);
    int num_samples = num_samples_values[0];
    offset = updated_offset;

    // Unpack channel_data
    auto [channel_data_values, final_offset, __] = to_ints(remaining_data, bit_width, num_samples, offset, signed_val);
    cd.channel_data = channel_data_values;
    offset = final_offset;

    return cd;
}

ByteArray NDTPPayloadBroadband::pack() const {
    ByteArray payload;
    BitOffset offset = 0;

    for(const auto& channel : channels) {
        channel.pack(bit_width, signed_val, offset, payload);
    }

    return payload;
}

NDTPPayloadBroadband NDTPPayloadBroadband::unpack(const ByteArray& data, BitOffset& offset) {
    NDTPPayloadBroadband payload;
    payload.signed_val = false; // Adjust based on context
    payload.bit_width = 0;      // Adjust based on context
    payload.sample_rate = 0;    // Adjust based on context

    // Example unpack logic; adjust as per actual data layout
    // This requires knowing how many channels to unpack; potentially passed as an additional parameter
    // For demonstration, we'll assume this function is called appropriately with the correct offset

    // Placeholder: Implement unpacking based on your specific protocol

    return payload;
}

// Implementation of NDTPPayloadSpiketrain
ByteArray NDTPPayloadSpiketrain::pack() const {
    ByteArray payload;

    // Pack spike_counts as a list of 2-bit integers
    auto [packed_spikes, offset] = to_bytes(spike_counts, BIT_WIDTH, payload, 0, false);
    payload = packed_spikes;

    return payload;
}

NDTPPayloadSpiketrain NDTPPayloadSpiketrain::unpack(const ByteArray& data, BitOffset& offset) {
    NDTPPayloadSpiketrain spiketrain;
    // Unpack spike_counts
    auto [spike_values, new_offset, remaining_data] = to_ints(data, BIT_WIDTH, 0, offset, false);
    spiketrain.spike_counts = spike_values;
    offset = new_offset;

    return spiketrain;
}

// Implementation of NDTPMessage
ByteArray NDTPMessage::pack() const {
    ByteArray packed_header = header.pack();
    ByteArray packed_payload;

    if(header.data_type == DataType::kBroadband) {
        packed_payload = broadband_payload.pack();
    }
    else if(header.data_type == DataType::kSpiketrain) {
        packed_payload = spiketrain_payload.pack();
    }
    else {
        throw std::invalid_argument("Unknown data type");
    }

    // Concatenate header and payload
    ByteArray message = packed_header;
    message.insert(message.end(), packed_payload.begin(), packed_payload.end());

    // Calculate CRC16
    uint16_t crc = crc16(message);
    message.push_back(crc & 0xFF);
    message.push_back((crc >> 8) & 0xFF);

    return message;
}

NDTPMessage NDTPMessage::unpack(const ByteArray& data) {
    // Calculate expected sizes
    size_t header_size = 16; // Adjust based on NDTPHeader::pack implementation
    size_t crc_size = 2;

    if(data.size() < header_size + crc_size) {
        throw std::invalid_argument("Data too short to unpack NDTPMessage");
    }

    // Extract CRC
    uint16_t received_crc = data[data.size() - 2] | (data[data.size() - 1] << 8);

    // Extract header
    ByteArray header_data(data.begin(), data.begin() + header_size);
    NDTPHeader hdr = NDTPHeader::unpack(header_data);

    // Extract payload
    ByteArray payload_data(data.begin() + header_size, data.end() - crc_size);

    // Verify CRC
    ByteArray message_without_crc(data.begin(), data.end() - crc_size);
    if(crc16(message_without_crc) != received_crc) {
        throw std::invalid_argument("CRC16 verification failed");
    }

    // Populate NDTPMessage
    NDTPMessage msg;
    msg.header = hdr;

    if(hdr.data_type == DataType::kBroadband) {
        msg.broadband_payload = NDTPPayloadBroadband::unpack(payload_data, 0);
    }
    else if(hdr.data_type == DataType::kSpiketrain) {
        msg.spiketrain_payload = NDTPPayloadSpiketrain::unpack(payload_data, 0);
    }
    else {
        throw std::invalid_argument("Unknown data type");
    }

    return msg;
}

// CRC16 Implementation
uint16_t NDTPMessage::crc16(const ByteArray& data, uint16_t poly, uint16_t init) {
    uint16_t crc = init;

    for(auto byte : data) {
        crc ^= static_cast<uint16_t>(byte) << 8;
        for(int _ = 0; _ < 8; ++_) {
            if(crc & 0x8000) {
                crc = (crc << 1) ^ poly;
            }
            else {
                crc <<= 1;
            }
        }
    }

    return crc & 0xFFFF;
}

bool NDTPMessage::crc16_verify(const ByteArray& data, uint16_t crc) {
    return crc16(data) == crc;
}

} // namespace libndtp