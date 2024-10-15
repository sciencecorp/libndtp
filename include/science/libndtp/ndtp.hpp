// include/libndtp/Ndtp.hpp
#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "science/libndtp/utils.hpp"
#include "science/synapse/api/datatype.pb.h"

namespace constants {
    constexpr uint8_t kNDTPVersion = 0x01;
    constexpr size_t kNDTPHeaderSize = 12;
} // namespace constants

namespace libndtp {

/**
 * NDTPHeader represents the header of an NDTP message.
 */
struct NDTPHeader {
    uint8_t version = constants::kNDTPVersion;
    int data_type; // Assuming DataType is an enum defined in datatype.pb.h
    uint64_t timestamp;
    uint16_t seq_number;

    ByteArray pack() const;
    static NDTPHeader unpack(const ByteArray& data);

    bool operator==(const NDTPHeader& other) const {
        return data_type == other.data_type && timestamp == other.timestamp && seq_number == other.seq_number;
    }
    bool operator!=(const NDTPHeader& other) const {
        return !(*this == other);
    }
};

/**
 * NDTPPayloadBroadband represents broadband payload data.
 */
struct NDTPPayloadBroadband {
    /**
     * ChannelData holds data for a single channel.
     */
    struct ChannelData {
        uint32_t channel_id; // 24-bit
        std::vector<int> channel_data;

        bool operator==(const ChannelData& other) const {
            return channel_id == other.channel_id && channel_data == other.channel_data;
        }
        bool operator!=(const ChannelData& other) const {
            return !(*this == other);
        }
    };

    bool is_signed; // 1 bit
    uint8_t bit_width; // 7 bits
    uint32_t ch_count; // 3 bytes
    uint16_t sample_rate; // 2 bytes
    std::vector<ChannelData> channels;

    ByteArray pack() const;
    static NDTPPayloadBroadband unpack(const ByteArray& data);

    bool operator==(const NDTPPayloadBroadband& other) const {
        return is_signed == other.is_signed && bit_width == other.bit_width  && sample_rate == other.sample_rate && channels == other.channels;
    }
    bool operator!=(const NDTPPayloadBroadband& other) const {
        return !(*this == other);
    }
};

/**
 * NDTPPayloadSpiketrain represents spiketrain payload data.
 */
struct NDTPPayloadSpiketrain {
    static constexpr int BIT_WIDTH = 2;
    std::vector<int> spike_counts;

    // Packs the spiketrain payload into a byte array.
    ByteArray pack() const;

    // Unpacks the spiketrain payload from a byte array.
    static NDTPPayloadSpiketrain unpack(const ByteArray& data, BitOffset& offset);
};

/**
 * NDTPMessage represents a complete NDTP message, including header and payload.
 */
struct NDTPMessage {
    NDTPHeader header;
    NDTPPayloadBroadband broadband_payload;
    NDTPPayloadSpiketrain spiketrain_payload;

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

} // namespace libndtp
