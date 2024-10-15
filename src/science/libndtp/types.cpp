#include "science/libndtp/types.h"
#include "science/libndtp/ndtp.h"

namespace science::libndtp {

// Implementation of ElectricalBroadbandData
std::vector<ByteArray> ElectricalBroadbandData::pack(int seq_number) const {
  std::vector<ByteArray> packets;
  int seq_number_offset = 0;

  for (const auto& channel : channels) {
    NDTPHeader header;
    header.version = NDTP_VERSION;
    header.data_type = synapse::DataType::kBroadband;
    header.timestamp = t0;
    header.seq_number = seq_number + seq_number_offset;

    NDTPPayloadBroadband payload;
    payload.is_signed = is_signed;
    payload.bit_width = bit_width;
    payload.sample_rate = sample_rate;
    payload.channels.push_back({.channel_id = channel.channel_id, .channel_data = channel.channel_data});

    NDTPMessage message;
    message.header = header;
    message.payload = payload;

    packets.push_back(message.pack());
    seq_number_offset += 1;
  }

  return packets;
}

ElectricalBroadbandData ElectricalBroadbandData::unpack(const NDTPMessage& msg) {
  ElectricalBroadbandData data;
  auto payload = std::get<NDTPPayloadBroadband>(msg.payload);
  data.bit_width = payload.bit_width;
  data.is_signed = payload.is_signed;
  data.sample_rate = payload.sample_rate;
  data.t0 = msg.header.timestamp;

  for (const auto& channel : payload.channels) {
    ChannelData cd;
    cd.channel_id = channel.channel_id;
    cd.channel_data = channel.channel_data;
    data.channels.push_back(cd);
  }

  return data;
}

// Implementation of BinnedSpiketrainData
std::vector<ByteArray> BinnedSpiketrainData::pack(int seq_number) const {
  std::vector<ByteArray> packets;

  NDTPHeader header;
  header.version = NDTP_VERSION;
  header.data_type = synapse::DataType::kSpiketrain;
  header.timestamp = 0;  // Assign appropriate timestamp
  header.seq_number = seq_number;

  NDTPPayloadSpiketrain payload;
  payload.spike_counts = spike_counts;

  NDTPMessage message;
  message.header = header;
  message.payload = payload;

  packets.push_back(message.pack());

  return packets;
}

BinnedSpiketrainData BinnedSpiketrainData::unpack(const NDTPMessage& msg) {
  BinnedSpiketrainData data;
  data.spike_counts = std::get<NDTPPayloadSpiketrain>(msg.payload).spike_counts;
  return data;
}

}  // namespace science::libndtp
