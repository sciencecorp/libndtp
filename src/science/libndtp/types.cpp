#include "science/libndtp/types.h"
#include "science/libndtp/ndtp.h"

namespace science::libndtp {

static constexpr size_t MAX_CH_PAYLOAD_SIZE_BYTES = 1400;

void chunk_channel_data(
  const std::vector<uint64_t>& ch_data,
  size_t max_payload_size_bytes,
  std::vector<std::vector<uint64_t>>* chunks
) {
  size_t n_packets = std::ceil(ch_data.size() / max_payload_size_bytes);
  size_t n_pts_per_packet = std::ceil(ch_data.size() / n_packets);
  for (size_t i = 0; i < n_packets; ++i) {
    size_t start_idx = i * n_pts_per_packet;
    size_t end_idx = std::min(start_idx + n_pts_per_packet, ch_data.size());
    chunks->push_back(std::vector<uint64_t>(ch_data.begin() + start_idx, ch_data.begin() + end_idx));
  }
}

std::vector<ByteArray> ElectricalBroadbandData::pack(uint64_t seq_number) const {
  std::vector<ByteArray> packets;
  int seq_number_offset = 0;

  for (const auto& channel : channels) {
    std::vector<std::vector<uint64_t>> chunked;
    chunk_channel_data(channel.channel_data, MAX_CH_PAYLOAD_SIZE_BYTES, &chunked);

    for (const auto& chunk : chunked) {
      NDTPHeader header;
      header.version = NDTP_VERSION;
      header.data_type = synapse::DataType::kBroadband;
      header.timestamp = t0;
      header.seq_number = seq_number + seq_number_offset;

      NDTPPayloadBroadband payload;
      payload.is_signed = is_signed;
      payload.bit_width = bit_width;
      payload.sample_rate = sample_rate;
      payload.channels.push_back({
        .channel_id = channel.channel_id,
        .channel_data = channel.channel_data
      });

      NDTPMessage message;
      message.header = header;
      message.payload = payload;

      packets.push_back(message.pack());
      seq_number_offset += 1;
    }
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
std::vector<ByteArray> BinnedSpiketrainData::pack(uint64_t seq_number) const {
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
  data.bin_size_ms = std::get<NDTPPayloadSpiketrain>(msg.payload).bin_size_ms;
  data.t0 = msg.header.timestamp;
  return data;
}

}  // namespace science::libndtp
