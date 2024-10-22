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
struct NDTPPayloadBroadband {
  struct ChannelData {
    uint32_t channel_id;  // 24-bit
    std::vector<uint64_t> channel_data;

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

  ByteArray pack() const;
  static NDTPPayloadBroadband unpack(const ByteArray& data);

  bool operator==(const NDTPPayloadBroadband& other) const {
    return is_signed == other.is_signed &&
            bit_width == other.bit_width &&
            sample_rate == other.sample_rate &&
            channels == other.channels;
  }

  bool operator!=(const NDTPPayloadBroadband& other) const { return !(*this == other); }
};

/**
 * NDTPPayloadSpiketrain represents spiketrain payload data.
 */
struct NDTPPayloadSpiketrain {
  static constexpr uint8_t BIT_WIDTH_BINNED_SPIKES = 2;

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
