// src/Ndtp.cpp
#include "science/libndtp/ndtp.hpp"
#include <iostream>
#include "science/libndtp/utils.hpp"

namespace libndtp {

ByteArray NDTPHeader::pack() const {
  ByteArray data(kNDTPHeaderSize);
  uint8_t* ptr = data.data();

  *ptr++ = version;
  *ptr++ = data_type;  // enum DataType

  std::memcpy(ptr, &timestamp, sizeof(timestamp));
  ptr += sizeof(timestamp);
  std::memcpy(ptr, &seq_number, sizeof(seq_number));
  ptr += sizeof(seq_number);

  return data;
}

NDTPHeader NDTPHeader::unpack(const ByteArray& data) {
  if (data.size() < kNDTPHeaderSize) {
    throw std::invalid_argument(
        "invalid header size: expected " + std::to_string(kNDTPHeaderSize) + ", got " + std::to_string(data.size())
    );
  }
  const uint8_t* ptr = data.data();

  uint8_t version = *ptr++;
  if (version != kNDTPVersion) {
    throw std::invalid_argument(
        "invalid version: expected " + std::to_string(kNDTPVersion) + ", got " + std::to_string(version)
    );
  }

  uint8_t data_type = *ptr++;
  uint64_t timestamp;
  std::memcpy(&timestamp, ptr, sizeof(timestamp));
  ptr += sizeof(timestamp);
  uint16_t seq_number = 0;
  std::memcpy(&seq_number, ptr, sizeof(seq_number));

  return NDTPHeader{version, data_type, timestamp, seq_number};
}

ByteArray NDTPPayloadBroadband::pack() const {
  ByteArray payload;
  uint32_t n_channels = channels.size();

  // First byte: bit width and signed flag
  payload.push_back(((bit_width & 0x7F) << 1) | (is_signed ? 1 : 0));

  // Next three bytes: number of channels (24-bit integer)
  payload.push_back((n_channels >> 16) & 0xFF);
  payload.push_back((n_channels >> 8) & 0xFF);
  payload.push_back(n_channels & 0xFF);

  // Next two bytes: sample rate (16-bit integer)
  payload.push_back((sample_rate >> 8) & 0xFF);
  payload.push_back(sample_rate & 0xFF);

  for (const auto& c : channels) {
    // Pack channel_id (3 bytes, 24 bits)
    payload.push_back((c.channel_id >> 16) & 0xFF);
    payload.push_back((c.channel_id >> 8) & 0xFF);
    payload.push_back(c.channel_id & 0xFF);

    // Pack number of samples (2 bytes, 16 bits)
    uint16_t num_samples = c.channel_data.size();
    payload.push_back((num_samples >> 8) & 0xFF);
    payload.push_back(num_samples & 0xFF);

    // Pack channel_data
    auto [channel_data_bytes, final_bit_offset] = to_bytes(c.channel_data, bit_width, {}, 0, is_signed);
    payload.insert(payload.end(), channel_data_bytes.begin(), channel_data_bytes.end());
  }

  return payload;
}

NDTPPayloadBroadband NDTPPayloadBroadband::unpack(const ByteArray& data) {
  if (data.size() < 6) {
    throw std::runtime_error("Invalid data size for NDTPPayloadBroadband");
  }
  uint8_t bit_width = data[0] >> 1;
  bool is_signed = (data[0] & 1) == 1;
  uint32_t num_channels = (data[1] << 16) | (data[2] << 8) | data[3];
  uint16_t sample_rate = (data[4] << 8) | data[5];

  BitOffset offset = 6;
  std::vector<NDTPPayloadBroadband::ChannelData> channels;
  for (uint32_t i = 0; i < num_channels; ++i) {
    if (offset + 3 > data.size()) {
      throw std::runtime_error("Incomplete data for channel_id");
    }
    // unpack channel_id (3 bytes, big-endian)
    uint32_t channel_id = (data[offset] << 16) | (data[offset + 1] << 8) | data[offset + 2];
    offset += 3;

    // unpack num_samples (2 bytes, big-endian)
    uint16_t num_samples = (data[offset] << 8) | data[offset + 1];
    offset += 2;

    // calculate the number of bytes needed for the channel data
    uint32_t num_bytes = (num_samples * bit_width + 7) / 8;

    // ensure we have enough data
    if (offset + num_bytes > data.size()) {
      throw std::runtime_error("Incomplete data for channel_data");
    }
    std::vector<uint8_t> channel_data_bytes(data.begin() + offset, data.begin() + offset + num_bytes);
    offset += num_bytes;

    // unpack channel_data
    auto [channel_data, end_bit, truncated_data] =
        to_ints(channel_data_bytes, bit_width, num_samples, 0, is_signed, false);
    channels.emplace_back(NDTPPayloadBroadband::ChannelData{.channel_id = channel_id, .channel_data = channel_data});
  }
  return NDTPPayloadBroadband{
      .is_signed = is_signed, .bit_width = bit_width, .sample_rate = sample_rate, .channels = channels
  };
}

// Implementation of NDTPPayloadSpiketrain
ByteArray NDTPPayloadSpiketrain::pack() const {
  int num_counts = spike_counts.size();
  int clamp_value = (1 << kSpikeTrainBitWidth) - 1;
  std::vector<int> clamped_counts;

  // clamp spike counts to max value allowed by bit width
  for (const int& count : spike_counts) {
    clamped_counts.push_back(std::min(count, clamp_value));
  }

  ByteArray result(4);  // Start with 4 bytes for num_counts
  // pack sample count (4 bytes, little-endian)
  result[0] = num_counts & 0xFF;
  result[1] = (num_counts >> 8) & 0xFF;
  result[2] = (num_counts >> 16) & 0xFF;
  result[3] = (num_counts >> 24) & 0xFF;

  // pack clamped spike counts
  auto [bytes, final_bit_offset] = to_bytes(clamped_counts, kSpikeTrainBitWidth, {}, 0, false);
  result.insert(result.end(), bytes.begin(), bytes.end());

  return result;
}

NDTPPayloadSpiketrain NDTPPayloadSpiketrain::unpack(const ByteArray& data) {
  if (data.size() < 4) {
    throw std::runtime_error("Invalid data size for NDTPPayloadSpiketrain");
  }
  uint32_t num_counts = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

  std::vector<uint8_t> payload(data.begin() + 4, data.end());
  auto bits_needed = num_counts * kSpikeTrainBitWidth;
  auto bytes_needed = (bits_needed + 7) / 8;
  if (payload.size() < bytes_needed) {
    throw std::runtime_error("Insufficient data for spike_counts");
  }
  payload.resize(bytes_needed);
  std::vector<int> spike_counts;
  std::tie(spike_counts, std::ignore, std::ignore) = to_ints(payload, kSpikeTrainBitWidth, num_counts, 0, false);
  return NDTPPayloadSpiketrain{.spike_counts = spike_counts};
}

// // Implementation of NDTPMessage
// ByteArray NDTPMessage::pack() const {
//     ByteArray packed_header = header.pack();
//     ByteArray packed_payload;

//     if(header.data_type == synapse::DataType::kBroadband) {
//         packed_payload = broadband_payload.pack();
//     }
//     else if(header.data_type == synapse::DataType::kSpiketrain) {
//         packed_payload = spiketrain_payload.pack();
//     }
//     else {
//         throw std::invalid_argument("Unknown data type");
//     }

//     // Concatenate header and payload
//     ByteArray message = packed_header;
//     message.insert(message.end(), packed_payload.begin(), packed_payload.end());

//     // Calculate CRC16
//     uint16_t crc = crc16(message);
//     message.push_back(crc & 0xFF);
//     message.push_back((crc >> 8) & 0xFF);

//     return message;
// }

// NDTPMessage NDTPMessage::unpack(const ByteArray& data) {
//     // Calculate expected sizes
//     size_t header_size = 16; // Adjust based on NDTPHeader::pack implementation
//     size_t crc_size = 2;

//     if(data.size() < header_size + crc_size) {
//         throw std::invalid_argument("Data too short to unpack NDTPMessage");
//     }

//     // Extract CRC
//     uint16_t received_crc = data[data.size() - 2] | (data[data.size() - 1] << 8);

//     // Extract header
//     ByteArray header_data(data.begin(), data.begin() + header_size);
//     NDTPHeader hdr = NDTPHeader::unpack(header_data);

//     // Extract payload
//     ByteArray payload_data(data.begin() + header_size, data.end() - crc_size);

//     // Verify CRC
//     ByteArray message_without_crc(data.begin(), data.end() - crc_size);
//     if(crc16(message_without_crc) != received_crc) {
//         throw std::invalid_argument("CRC16 verification failed");
//     }

//     // Populate NDTPMessage
//     NDTPMessage msg;
//     msg.header = hdr;

//     if(hdr.data_type == synapse::DataType::kBroadband) {
//         msg.broadband_payload = NDTPPayloadBroadband::unpack(payload_data, 0);
//     }
//     else if(hdr.data_type == synapse::DataType::kSpiketrain) {
//         msg.spiketrain_payload = NDTPPayloadSpiketrain::unpack(payload_data, 0);
//     }
//     else {
//         throw std::invalid_argument("Unknown data type");
//     }

//     return msg;
// }

// // CRC16 Implementation
// uint16_t NDTPMessage::crc16(const ByteArray& data, uint16_t poly, uint16_t init) {
//     uint16_t crc = init;

//     for(auto byte : data) {
//         crc ^= static_cast<uint16_t>(byte) << 8;
//         for(int _ = 0; _ < 8; ++_) {
//             if(crc & 0x8000) {
//                 crc = (crc << 1) ^ poly;
//             }
//             else {
//                 crc <<= 1;
//             }
//         }
//     }

//     return crc & 0xFFFF;
// }

// bool NDTPMessage::crc16_verify(const ByteArray& data, uint16_t crc) {
//     return crc16(data) == crc;
// }

}  // namespace libndtp
