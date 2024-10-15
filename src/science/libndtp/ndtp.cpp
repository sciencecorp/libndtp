// src/Ndtp.cpp
#include "science/libndtp/ndtp.h"
#include <bit>
#include <cstring>
#include <iostream>
#include "science/libndtp/utils.h"


namespace science::libndtp {


std::vector<int64_t> to_signed(const std::vector<uint64_t>& unsigned_vec) {
    std::vector<int64_t> signed_vec(unsigned_vec.size());
    for (size_t i = 0; i < unsigned_vec.size(); ++i) {
        std::memcpy(&signed_vec[i], &unsigned_vec[i], sizeof(uint64_t));
    }
    return signed_vec;
}

std::vector<uint64_t> to_unsigned(const std::vector<int64_t>& signed_vec) {
    std::vector<uint64_t> unsigned_vec(signed_vec.size());
    for (size_t i = 0; i < signed_vec.size(); ++i) {
        std::memcpy(&unsigned_vec[i], &signed_vec[i], sizeof(int64_t));
    }
    return unsigned_vec;
}

ByteArray NDTPHeader::pack() const {
  ByteArray data(NDTP_HEADER_SIZE);
  uint8_t* ptr = data.data();

  *ptr++ = version;
  *ptr++ = data_type;

  uint64_t n_timestamp = htonll(timestamp);
  std::memcpy(ptr, &n_timestamp, sizeof(n_timestamp));
  ptr += sizeof(n_timestamp);

  uint16_t n_seq_number = htons(seq_number);
  std::memcpy(ptr, &n_seq_number, sizeof(n_seq_number));
  ptr += sizeof(n_seq_number);

  return data;
}

NDTPHeader NDTPHeader::unpack(const ByteArray& data) {
  if (data.size() < NDTP_HEADER_SIZE) {
    throw std::invalid_argument(
        "invalid header size: expected " + std::to_string(NDTP_HEADER_SIZE) + ", got " + std::to_string(data.size())
    );
  }
  const uint8_t* ptr = data.data();

  uint8_t version = *ptr++;
  if (version != NDTP_VERSION) {
    throw std::invalid_argument(
        "invalid version: expected " + std::to_string(NDTP_VERSION) + ", got " + std::to_string(version)
    );
  }

  uint8_t data_type = *ptr++;

  uint64_t n_timestamp;
  std::memcpy(&n_timestamp, ptr, sizeof(n_timestamp));
  ptr += sizeof(n_timestamp);

  uint16_t n_seq_number = 0;
  std::memcpy(&n_seq_number, ptr, sizeof(n_seq_number));

  return NDTPHeader{version, data_type, ntohll(n_timestamp), ntohs(n_seq_number)};
}

ByteArray NDTPPayloadBroadband::pack() const {
  ByteArray payload;

  // First byte: bit width and signed flag
  payload.push_back(((bit_width & 0x7F) << 1) | (is_signed ? 1 : 0));

  // Next three bytes: number of channels (3 bytes) (big endian)
  uint32_t n_channels = htonl(channels.size());

  payload.push_back((n_channels >> 24) & 0xFF);
  payload.push_back((n_channels >> 16) & 0xFF);
  payload.push_back((n_channels >> 8) & 0xFF);

  // Next two bytes: sample rate (2 bytes)
  uint16_t n_sample_rate = htons(sample_rate);
  payload.push_back((n_sample_rate >> 8) & 0xFF);
  payload.push_back(n_sample_rate & 0xFF);

  for (const auto& c : channels) {
    uint32_t n_channel_id = htonl(c.channel_id);

    // Pack channel_id (3 bytes)
    payload.push_back((n_channel_id >> 24) & 0xFF);
    payload.push_back((n_channel_id >> 16) & 0xFF);
    payload.push_back((n_channel_id >> 8) & 0xFF);

    // Pack number of samples (2 bytes)
    uint16_t num_samples = c.channel_data.size();
    uint16_t n_num_samples = htons(num_samples);
    payload.push_back((n_num_samples >> 8) & 0xFF);
    payload.push_back(n_num_samples & 0xFF);

    // Pack channel_data
    if (is_signed) {
      std::vector<int64_t> channel_data = to_signed(c.channel_data);

      auto res = to_bytes<int64_t>(channel_data, bit_width);
      auto channel_data_bytes = std::get<0>(res);
      payload.insert(payload.end(), channel_data_bytes.begin(), channel_data_bytes.end());
    } else {
      auto res = to_bytes<uint64_t>(c.channel_data, bit_width);
      auto channel_data_bytes = std::get<0>(res);
      payload.insert(payload.end(), channel_data_bytes.begin(), channel_data_bytes.end());
    }
  }

  return payload;
}

NDTPPayloadBroadband NDTPPayloadBroadband::unpack(const ByteArray& data) {
  if (data.size() < 6) {
    throw std::runtime_error("Invalid data size for NDTPPayloadBroadband");
  }
  uint8_t bit_width = data[0] >> 1;
  bool is_signed = (data[0] & 1) == 1;
  uint32_t num_channels = ntohl((data[1] << 24) | (data[2] << 16) | data[3] << 8);
  uint16_t sample_rate = (data[4] << 8) | data[5];

  BitOffset offset = 6;

  std::vector<NDTPPayloadBroadband::ChannelData> channels;
  for (uint32_t i = 0; i < num_channels; ++i) {
    if (offset + 3 > data.size()) {
      throw std::runtime_error("Incomplete data for channel_id");
    }
    // unpack channel_id (3 bytes)
    uint32_t channel_id = ntohl((data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8));
    offset += 3;

    // unpack num_samples (2 bytes)
    uint16_t num_samples = ntohs((data[offset] << 8) | data[offset + 1]);
    offset += 2;

    uint32_t num_bytes = (num_samples * bit_width + 7) / 8;

    if (offset + num_bytes > data.size()) {
      throw std::runtime_error("Incomplete data for channel_data");
    }
    std::vector<uint8_t> channel_data_bytes(data.begin() + offset, data.begin() + offset + num_bytes);
    offset += num_bytes;

    // unpack channel_data
    std::vector<uint64_t> channel_data;
    if (is_signed) {
      auto res = to_ints<int64_t>(channel_data_bytes, bit_width, num_samples, 0, is_signed);
      auto channel_data_signed = std::get<0>(res);
      channel_data = to_unsigned(channel_data_signed);
    } else {
      auto res = to_ints<uint64_t>(channel_data_bytes, bit_width, num_samples, 0, is_signed);
      channel_data = std::get<0>(res);
    }

    channels.emplace_back(NDTPPayloadBroadband::ChannelData{
      .channel_id = channel_id,
      .channel_data = channel_data
    });
  }

  return NDTPPayloadBroadband{
      .is_signed = is_signed,
      .bit_width = bit_width,
      .ch_count = ntohl(num_channels),
      .sample_rate = ntohs(sample_rate),
      .channels = channels
  };
}

// Implementation of NDTPPayloadSpiketrain
ByteArray NDTPPayloadSpiketrain::pack() const {
  size_t sample_count = spike_counts.size();
  uint8_t clamp_value = (1 << BIT_WIDTH_BINNED_SPIKES) - 1;
  std::vector<uint64_t> clamped_counts;

  // clamp spike counts to max value allowed by bit width
  for (const auto& count : spike_counts) {
    clamped_counts.push_back(std::min(static_cast<uint8_t>(count), clamp_value));
  }

  ByteArray result;  // Start with 4 bytes for sample_count

  // Pack sample_count (4 bytes)
  uint32_t n_num_counts = htonl(sample_count);
  result.push_back((n_num_counts >> 24) & 0xFF);
  result.push_back((n_num_counts >> 16) & 0xFF);
  result.push_back((n_num_counts >> 8) & 0xFF);
  result.push_back(n_num_counts & 0xFF);

  // Pack bin_size_ms (1 byte)
  result.push_back(bin_size_ms);

  // pack clamped spike counts
  auto [bytes, final_bit_offset] = to_bytes(clamped_counts, BIT_WIDTH_BINNED_SPIKES, {}, 0);
  result.insert(result.end(), bytes.begin(), bytes.end());

  return result;
}

NDTPPayloadSpiketrain NDTPPayloadSpiketrain::unpack(const ByteArray& data) {
  if (data.size() < 4) {
    throw std::runtime_error("Invalid data size for NDTPPayloadSpiketrain");
  }

  // unpack sample_count (4 bytes)
  uint32_t sample_count = ntohl(data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);
  std::cout << "expecting " << sample_count << " samples" << std::endl;

  // unpack bin_size_ms (1 byte)
  uint8_t bin_size_ms = data[4];

  // unpack spike_counts
  std::vector<uint8_t> payload(data.begin() + 5, data.end());
  auto bits_needed = sample_count * BIT_WIDTH_BINNED_SPIKES;
  auto bytes_needed = (bits_needed + 7) / 8;
  if (payload.size() < bytes_needed) {
    throw std::runtime_error(
      "insufficient data for spike_count (expected " + std::to_string(bytes_needed) +
      ", got " + std::to_string(payload.size()) + ")"
    );
  }

  payload.resize(bytes_needed);
  std::vector<uint8_t> spike_counts;
  std::tie(spike_counts, std::ignore, std::ignore) = to_ints<uint8_t>(
    payload, BIT_WIDTH_BINNED_SPIKES, sample_count
  );

  return NDTPPayloadSpiketrain {
    .bin_size_ms = bin_size_ms,
    .spike_counts = spike_counts
  };
}

ByteArray NDTPMessage::pack() const {
  auto result = header.pack();

  if (std::holds_alternative<NDTPPayloadBroadband>(payload)) {
    auto payload_bytes = std::get<NDTPPayloadBroadband>(payload).pack();
    result.insert(result.end(), payload_bytes.begin(), payload_bytes.end());

  } else if (std::holds_alternative<NDTPPayloadSpiketrain>(payload)) {
    auto payload_bytes = std::get<NDTPPayloadSpiketrain>(payload).pack();
    result.insert(result.end(), payload_bytes.begin(), payload_bytes.end());

  } else {
    throw std::runtime_error("Unsupported payload type");
  }

  uint16_t crc = htons(crc16(result));
  result.push_back((crc >> 8) & 0xFF);
  result.push_back(crc & 0xFF);

  return result;
}

NDTPMessage NDTPMessage::unpack(const ByteArray& data) {
  if (data.size() < 16) {
    throw std::runtime_error("invalid data size for NDTPMessage");
  }

  auto data_bytes = std::vector<uint8_t>(data.begin(), data.end() - 2);
  auto header_bytes = std::vector<uint8_t>(data.begin(), data.begin() + NDTPHeader::NDTP_HEADER_SIZE);
  auto payload_bytes = std::vector<uint8_t>(data.begin() + NDTPHeader::NDTP_HEADER_SIZE, data.end() - 2);

  uint16_t received_crc = ntohs((data[data.size() - 2] << 8) | data[data.size() - 1]);
  if (!crc16_verify(data_bytes, received_crc)) {
    throw std::runtime_error(
      "CRC verification failed (expected " + std::to_string(received_crc) +
      ", got " + std::to_string(crc16(data_bytes)) + ")"
    );
  }

  auto header = NDTPHeader::unpack(header_bytes);
  if (header.data_type == synapse::DataType::kBroadband) {
    auto unpacked_payload = NDTPPayloadBroadband::unpack(payload_bytes);
    return NDTPMessage{.header = header, .payload = unpacked_payload};
  } else if (header.data_type == synapse::DataType::kSpiketrain) {
    auto unpacked_payload = NDTPPayloadSpiketrain::unpack(payload_bytes);
    return NDTPMessage{.header = header, .payload = unpacked_payload};
  }

  throw std::runtime_error("unsupported data type in NDTP header");
}

uint16_t NDTPMessage::crc16(const ByteArray& data, uint16_t poly, uint16_t init) {
  uint16_t crc = init;
  for (const auto& byte : data) {
    crc ^= byte << 8;
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ poly;
      } else {
        crc <<= 1;
      }
      crc &= 0xFFFF;
    }
  }
  return crc & 0xFFFF;
}

bool NDTPMessage::crc16_verify(const ByteArray& data, uint16_t crc) {
  return crc16(data) == crc;
}

}  // namespace science::libndtp
