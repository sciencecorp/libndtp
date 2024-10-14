// include/libndtp/Ndtp.hpp
#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "science/libndtp/utils.hpp"
#include "science/synapse/api/datatype.pb.h"

namespace libndtp {

/**
 * NDTPHeader represents the header of an NDTP message.
 */
struct NDTPHeader {
    uint8_t version;
    int data_type; // Assuming DataType is an enum defined in datatype.pb.h
    uint64_t timestamp;
    uint16_t seq_number;

    // Packs the header into a byte array.
    ByteArray pack() const;

    // Unpacks the header from a byte array.
    static NDTPHeader unpack(const ByteArray& data);
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

        // Packs the channel data into the payload.
        void pack(int bit_width, bool signed_val, BitOffset& offset, ByteArray& payload) const;

        // Unpacks the channel data from the payload.
        static ChannelData unpack(const ByteArray& data, int bit_width, bool signed_val, BitOffset& offset);
    };

    bool signed_val;
    int bit_width;
    int sample_rate;
    std::vector<ChannelData> channels;

    // Packs the broadband payload into a byte array.
    ByteArray pack() const;

    // Unpacks the broadband payload from a byte array.
    static NDTPPayloadBroadband unpack(const ByteArray& data, BitOffset& offset);
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
