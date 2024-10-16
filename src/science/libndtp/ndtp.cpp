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

  // Next three bytes: number of channels
  uint32_t n_channels = channels.size();
  payload.push_back((n_channels >> 16) & 0xFF);
  payload.push_back((n_channels >> 8) & 0xFF);
  payload.push_back((n_channels) & 0xFF);

  // Next three bytes: sample rate
  uint32_t n_sample_rate = sample_rate;
  payload.push_back((n_sample_rate >> 16) & 0xFF);
  payload.push_back((n_sample_rate >> 8) & 0xFF);
  payload.push_back(n_sample_rate & 0xFF);

  size_t bit_offset = 0;
  for (const auto& c : channels) {
    size_t num_samples = c.channel_data.size();
    if (num_samples > 0xFFFF) {
      throw std::runtime_error("number of samples is too large, must be less than 65536");
    }

    auto p_cid = to_bytes<uint32_t>({c.channel_id}, 24, payload, bit_offset);
    payload = std::get<0>(p_cid);
    bit_offset = std::get<1>(p_cid);

    auto p_num_samples = to_bytes<uint16_t>({static_cast<uint16_t>(num_samples)}, 16, payload, bit_offset);
    payload = std::get<0>(p_num_samples);
    bit_offset = std::get<1>(p_num_samples);

    auto p_channel_data = to_bytes<uint64_t>(c.channel_data, bit_width, payload, bit_offset, is_signed);
    payload = std::get<0>(p_channel_data);
    bit_offset = std::get<1>(p_channel_data);
  }

  return payload;
}

NDTPPayloadBroadband NDTPPayloadBroadband::unpack(const ByteArray& data) {
  if (data.size() < 7) {
    throw std::runtime_error("Invalid data size for NDTPPayloadBroadband");
  }
  uint8_t bit_width = data[0] >> 1;
  bool is_signed = (data[0] & 1) == 1;
  uint32_t num_channels = (data[1] << 16) | (data[2] << 8) | (data[3]);
  uint32_t sample_rate = (data[4] << 16) | (data[5] << 8) | (data[6]);

  BitOffset offset = 0;
  ByteArray truncated = std::vector<uint8_t>(data.begin() + 7, data.end());
  std::vector<NDTPPayloadBroadband::ChannelData> channels;
  for (uint32_t i = 0; i < num_channels; ++i) {
    auto u_channel_id = to_ints<uint32_t>(truncated, 24, 1, offset);
    uint32_t channel_id = std::get<0>(u_channel_id)[0];
    offset = std::get<1>(u_channel_id);
    truncated = std::get<2>(u_channel_id);

    auto u_num_samples = to_ints<uint16_t>(truncated, 16, 1, offset);
    uint16_t num_samples = std::get<0>(u_num_samples)[0];
    offset = std::get<1>(u_num_samples);
    truncated = std::get<2>(u_num_samples);

    auto u_channel_data = to_ints<uint64_t>(truncated, bit_width, num_samples, offset, is_signed);
    std::vector<uint64_t> channel_data = std::get<0>(u_channel_data);
    offset = std::get<1>(u_channel_data);
    truncated = std::get<2>(u_channel_data);

    channels.emplace_back(NDTPPayloadBroadband::ChannelData{
      .channel_id = channel_id,
      .channel_data = channel_data
    });
  }

  return NDTPPayloadBroadband{
      .is_signed = is_signed,
      .bit_width = bit_width,
      .ch_count = num_channels,
      .sample_rate = sample_rate,
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
  uint32_t n_num_counts = sample_count;
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
  if (data.size() < 5) {
    throw std::runtime_error("Invalid data size for NDTPPayloadSpiketrain");
  }

  // unpack sample_count (4 bytes)
  uint32_t sample_count = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];

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

ByteArray NDTPMessage::pack() {
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

  _crc16 = crc16(result);
  result.push_back((_crc16 >> 8) & 0xFF);
  result.push_back(_crc16 & 0xFF);

  return result;
}

NDTPMessage NDTPMessage::unpack(const ByteArray& data) {
  if (data.size() < 16) {
    throw std::runtime_error("invalid data size for NDTPMessage");
  }

  auto data_bytes = std::vector<uint8_t>(data.begin(), data.end() - 2);
  auto header_bytes = std::vector<uint8_t>(data.begin(), data.begin() + NDTPHeader::NDTP_HEADER_SIZE);
  auto payload_bytes = std::vector<uint8_t>(data.begin() + NDTPHeader::NDTP_HEADER_SIZE, data.end() - 2);

  uint16_t received_crc = (data[data.size() - 2] << 8) | data[data.size() - 1];
  if (!crc16_verify(data_bytes, received_crc)) {
    throw std::runtime_error(
      "CRC verification failed (expected " + std::to_string(received_crc) +
      ", got " + std::to_string(crc16(data_bytes)) + ")"
    );
  }

  auto header = NDTPHeader::unpack(header_bytes);
  if (header.data_type == synapse::DataType::kBroadband) {
    auto unpacked_payload = NDTPPayloadBroadband::unpack(payload_bytes);
    return NDTPMessage{ .header = header, .payload = unpacked_payload, ._crc16 = received_crc };
  } else if (header.data_type == synapse::DataType::kSpiketrain) {
    auto unpacked_payload = NDTPPayloadSpiketrain::unpack(payload_bytes);
    return NDTPMessage{ .header = header, .payload = unpacked_payload, ._crc16 = received_crc };
  }

  throw std::runtime_error("unsupported data type in NDTP header");
}

bool NDTPMessage::crc16_verify(const ByteArray& data, uint16_t crc) {
  return crc16(data) == crc;
}

}  // namespace science::libndtp
