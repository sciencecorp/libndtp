// include/libndtp/Ndtp.h
#pragma once

#include <cstdint>
#include <stdexcept>
#include <variant>
#include <vector>
#include "science/libndtp/utils.h"
#include "science/libndtp/synapse/api/datatype.pb.h"

#ifdef __linux__
#include <netinet/in.h>
#define htonll(x) ((1 == htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

namespace science::libndtp {

static constexpr uint8_t NDTP_VERSION = 0x01;

/**
 * NDTPHeader represents the header of an NDTP message.
 */
struct NDTPHeader {
  static constexpr size_t NDTP_HEADER_SIZE = 12;

  uint8_t version = NDTP_VERSION;
  uint8_t data_type;
  uint64_t timestamp;
  uint16_t seq_number;

  ByteArray pack() const;
  static NDTPHeader unpack(const ByteArray& data);

  bool operator==(const NDTPHeader& other) const {
    return data_type == other.data_type &&
           timestamp == other.timestamp &&
           seq_number == other.seq_number;
  }
  bool operator!=(const NDTPHeader& other) const { return !(*this == other); }
};

/**
 * NDTPPayloadBroadband represents broadband payload data.
 */
template <typename T>
struct GenericNDTPPayloadBroadband {

  struct ChannelData {
    uint32_t channel_id;  // 24-bit
    std::vector<T> channel_data;

    bool operator==(const ChannelData& other) const {
      return channel_id == other.channel_id && channel_data == other.channel_data;
    }

    bool operator!=(const ChannelData& other) const { return !(*this == other); }

  };

  bool is_signed;        // 1 bit
  uint8_t bit_width;     // 7 bits (combined with is_signed to form 8 bits)
  uint32_t ch_count;     // 3 bytes
  uint32_t sample_rate;  // 2 bytes
  std::vector<ChannelData> channels;

  static GenericNDTPPayloadBroadband<uint64_t> unpack(const ByteArray& data);

  bool operator==(const GenericNDTPPayloadBroadband& other) const {
    return is_signed == other.is_signed &&
            bit_width == other.bit_width &&
            sample_rate == other.sample_rate &&
            channels == other.channels;
  }

  bool operator!=(const GenericNDTPPayloadBroadband& other) const { return !(*this == other); }

  ByteArray pack() const {
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
      bit_offset = std::get<1>(p_cid);

      auto p_num_samples = to_bytes<uint16_t>({static_cast<uint16_t>(num_samples)}, 16, payload, bit_offset);
      bit_offset = std::get<1>(p_num_samples);

      auto p_channel_data = to_bytes<T>(c.channel_data, bit_width, payload, bit_offset, is_signed);
      bit_offset = std::get<1>(p_channel_data);
    }

    return payload;
  }

};

typedef GenericNDTPPayloadBroadband<uint64_t> NDTPPayloadBroadband;

/**
 * NDTPPayloadSpiketrain represents spiketrain payload data.
 */
struct NDTPPayloadSpiketrain {
  static constexpr uint8_t BIT_WIDTH_BINNED_SPIKES = 4;

  uint8_t bin_size_ms;                // 2 bits
  std::vector<uint8_t> spike_counts;  // 2 bits

  ByteArray pack() const;
  static NDTPPayloadSpiketrain unpack(const ByteArray& data);

  bool operator==(const NDTPPayloadSpiketrain& other) const {
    return spike_counts == other.spike_counts &&
            bin_size_ms == other.bin_size_ms;
  }
  bool operator!=(const NDTPPayloadSpiketrain& other) const { return !(*this == other); }
};

/**
 * NDTPMessage represents a complete NDTP message, including header and payload.
 */
struct NDTPMessage {
  NDTPHeader header;
  std::variant<NDTPPayloadBroadband, NDTPPayloadSpiketrain> payload;
  uint16_t _crc16;

  // Packs the entire message into a byte array, calculating the CRC16.
  ByteArray pack();

  // Unpacks the entire message from a byte array, verifying the CRC16.
  static NDTPMessage unpack(const ByteArray& data, bool ignore_crc = false);

 private:
  // Verifies CRC16 checksum.
  static bool crc16_verify(const ByteArray& data, uint16_t crc);
};

}  // namespace science::libndtp
