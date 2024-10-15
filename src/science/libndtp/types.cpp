#include "science/libndtp/types.hpp"
#include "science/libndtp/ndtp.hpp"

namespace libndtp {

// Implementation of ElectricalBroadbandData
std::vector<ByteArray> ElectricalBroadbandData::pack(int seq_number) const {
  std::vector<ByteArray> packets;
  int seq_number_offset = 0;

  for (const auto& channel : channels) {
    NDTPHeader header;
    header.version = kNDTPVersion;
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
    message.broadband_payload = payload;

    packets.push_back(message.pack());
    seq_number_offset += 1;
  }

  return packets;
}

ElectricalBroadbandData ElectricalBroadbandData::unpack(const NDTPMessage& msg) {
  ElectricalBroadbandData data;
  data.bit_width = msg.broadband_payload.bit_width;
  data.is_signed = msg.broadband_payload.is_signed;
  data.sample_rate = msg.broadband_payload.sample_rate;
  data.t0 = msg.header.timestamp;

  for (const auto& channel : msg.broadband_payload.channels) {
    ChannelData cd;
    cd.channel_id = channel.channel_id;
    cd.channel_data = channel.channel_data;
    data.channels.push_back(cd);
  }

  return data;
}

// Implementation of SpiketrainData
std::vector<ByteArray> SpiketrainData::pack(int seq_number) const {
  std::vector<ByteArray> packets;

  NDTPHeader header;
  header.version = kNDTPVersion;
  header.data_type = synapse::DataType::kSpiketrain;
  header.timestamp = 0;  // Assign appropriate timestamp
  header.seq_number = seq_number;

  NDTPPayloadSpiketrain payload;
  payload.spike_counts = spike_counts;

  NDTPMessage message;
  message.header = header;
  message.spiketrain_payload = payload;

  packets.push_back(message.pack());

  return packets;
}

SpiketrainData SpiketrainData::unpack(const NDTPMessage& msg) {
  SpiketrainData data;
  data.spike_counts = msg.spiketrain_payload.spike_counts;
  return data;
}

}  // namespace libndtp
