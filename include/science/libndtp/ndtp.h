// include/libndtp/Ndtp.h
#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>
#include "science/libndtp/utils.h"
#include "science/libndtp/synapse/api/datatype.pb.h"

namespace science::libndtp {

static constexpr uint8_t NDTP_VERSION = 0x01;

/**
 * NDTPHeader represents the header of an NDTP message.
 */
struct NDTPHeader {
  static constexpr size_t NDTP_HEADER_SIZE = 12;

  uint8_t version = NDTP_VERSION;
  int data_type;  // Assuming DataType is an enum defined in datatype.pb.h
  uint64_t timestamp;
  uint16_t seq_number;

  ByteArray pack() const;
  static NDTPHeader unpack(const ByteArray& data);

  bool operator==(const NDTPHeader& other) const {
    return data_type == other.data_type && timestamp == other.timestamp && seq_number == other.seq_number;
  }

  bool operator!=(const NDTPHeader& other) const { return !(*this == other); }
};

/**
 * NDTPPayloadBroadband represents broadband payload data.
 */
struct NDTPPayloadBroadband {
  /**
     * ChannelData holds data for a single channel.
     */
  struct ChannelData {
    uint32_t channel_id;  // 24-bit
    std::vector<int> channel_data;

    bool operator==(const ChannelData& other) const {
      return channel_id == other.channel_id && channel_data == other.channel_data;
    }

    bool operator!=(const ChannelData& other) const { return !(*this == other); }
  };

  bool is_signed;        // 1 bit
  uint8_t bit_width;     // 7 bits
  uint32_t ch_count;     // 3 bytes
  uint16_t sample_rate;  // 2 bytes
  std::vector<ChannelData> channels;

  ByteArray pack() const;
  static NDTPPayloadBroadband unpack(const ByteArray& data);

  bool operator==(const NDTPPayloadBroadband& other) const {
    return is_signed == other.is_signed && bit_width == other.bit_width && sample_rate == other.sample_rate &&
           channels == other.channels;
  }

  bool operator!=(const NDTPPayloadBroadband& other) const { return !(*this == other); }
};

/**
 * NDTPPayloadSpiketrain represents spiketrain payload data.
 */
struct NDTPPayloadSpiketrain {
  static constexpr uint8_t BIT_WIDTH_BINNED_SPIKES = 2;
  std::vector<int> spike_counts;

  ByteArray pack() const;
  static NDTPPayloadSpiketrain unpack(const ByteArray& data);

  bool operator==(const NDTPPayloadSpiketrain& other) const { return spike_counts == other.spike_counts; }

  bool operator!=(const NDTPPayloadSpiketrain& other) const { return !(*this == other); }
};

/**
 * NDTPMessage represents a complete NDTP message, including header and payload.
 */
struct NDTPMessage {
  NDTPHeader header;
  std::variant<NDTPPayloadBroadband, NDTPPayloadSpiketrain> payload;

  // Packs the entire message into a byte array, including CRC16.
  ByteArray pack() const;

  // Unpacks the entire message from a byte array, verifying CRC16.
  static NDTPMessage unpack(const ByteArray& data);

 private:
  // Calculates CRC16 checksum.
  static uint16_t crc16(const ByteArray& data, uint16_t poly = 0x8005, uint16_t init = 0xFFFF);

  // Verifies CRC16 checksum.
  static bool crc16_verify(const ByteArray& data, uint16_t crc);
};

}  // namespace science::libndtp
