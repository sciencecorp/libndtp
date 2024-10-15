// include/libndtp/Types.hpp
#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "science/libndtp/ndtp.hpp"
#include "science/libndtp/utils.hpp"

namespace libndtp {
/**
 * ElectricalBroadbandData represents a collection of broadband data channels.
 */
struct ElectricalBroadbandData {
    /**
     * ChannelData holds data for a single electrical broadband channel.
     */
    struct ChannelData {
        uint32_t channel_id;
        std::vector<int> channel_data;
    };

    int bit_width;
    bool is_signed;
    int sample_rate;
    uint64_t t0;
    std::vector<ChannelData> channels;

    // Packs the data into a list of NDTP messages.
    std::vector<ByteArray> pack(int seq_number) const;

    // Unpacks the data from NDTP messages.
    static ElectricalBroadbandData unpack(const NDTPMessage& msg);
};

/**
 * SpiketrainData represents spike count data.
 */
struct SpiketrainData {
    std::vector<int> spike_counts;

    // Packs the data into a list of NDTP messages.
    std::vector<ByteArray> pack(int seq_number) const;

    // Unpacks the data from NDTP messages.
    static SpiketrainData unpack(const NDTPMessage& msg);
};

// Alias for union type
using SynapseData = std::variant<ElectricalBroadbandData, SpiketrainData>;

} // namespace libndtp
