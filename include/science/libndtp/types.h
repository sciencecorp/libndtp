// include/libndtp/Types.h
#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>
#include "science/libndtp/ndtp.h"
#include "science/libndtp/utils.h"

namespace science::libndtp {

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
 * BinnedSpiketrainData represents spike count data.
 */
struct BinnedSpiketrainData {
  std::vector<int> spike_counts;

  // Packs the data into a list of NDTP messages.
  std::vector<ByteArray> pack(int seq_number) const;

  // Unpacks the data from NDTP messages.
  static BinnedSpiketrainData unpack(const NDTPMessage& msg);
};


// Alias for union type
using SynapseData = std::variant<ElectricalBroadbandData, BinnedSpiketrainData>;

}  // namespace science::libndtp
