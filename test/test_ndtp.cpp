#include <gtest/gtest.h>
#include <science/libndtp/ndtp.h>
#include <science/libndtp/types.h>

namespace science::libndtp {

TEST(NDTPTest, NDTPHeaderPackUnpack) {
  NDTPHeader header{.data_type = synapse::DataType::kBroadband, .timestamp = 1234567890, .seq_number = 42};
  auto packed = header.pack();
  auto unpacked = NDTPHeader::unpack(packed);
  EXPECT_TRUE(unpacked == header);

  // invalid version
  auto INVALID_VERSION = 0x02;
  std::vector<uint8_t> invalid_version_data;
  invalid_version_data.push_back(INVALID_VERSION);

  auto data_type = static_cast<std::underlying_type_t<synapse::DataType>>(synapse::DataType::kBroadband);
  invalid_version_data.insert(
      invalid_version_data.end(), reinterpret_cast<const uint8_t*>(&data_type),
      reinterpret_cast<const uint8_t*>(&data_type) + sizeof(synapse::DataType)
  );
  uint64_t timestamp = 123;
  invalid_version_data.insert(
      invalid_version_data.end(), reinterpret_cast<const uint8_t*>(&timestamp),
      reinterpret_cast<const uint8_t*>(&timestamp) + sizeof(uint64_t)
  );
  uint16_t seq_number = 42;
  invalid_version_data.insert(
      invalid_version_data.end(), reinterpret_cast<const uint8_t*>(&seq_number),
      reinterpret_cast<const uint8_t*>(&seq_number) + sizeof(uint16_t)
  );

  EXPECT_THROW(NDTPHeader::unpack(invalid_version_data), std::invalid_argument);

  // insufficient data size
  std::vector<uint8_t> insufficient_data;
  insufficient_data.push_back(NDTP_VERSION);
  insufficient_data.insert(
      insufficient_data.end(), reinterpret_cast<const uint8_t*>(&data_type),
      reinterpret_cast<const uint8_t*>(&data_type) + sizeof(uint8_t)
  );

  insufficient_data.insert(
      insufficient_data.end(), reinterpret_cast<const uint8_t*>(&timestamp),
      reinterpret_cast<const uint8_t*>(&timestamp) + sizeof(uint64_t)
  );

  EXPECT_THROW(NDTPHeader::unpack(insufficient_data), std::invalid_argument);
}

TEST(NDTPTest, NDTPPayloadBroadbandPackUnpack) {
  uint8_t bit_width = 12;
  uint16_t sample_rate = 3;
  bool is_signed = false;
  std::vector<NDTPPayloadBroadband::ChannelData> channels;
  channels.push_back({0, {1, 2, 3}});
  channels.push_back({1, {4, 5, 6}});
  channels.push_back({2, {3000, 2000, 1000}});

  NDTPPayloadBroadband payload{
    .is_signed = is_signed,
    .bit_width = bit_width,
    .sample_rate = sample_rate,
    .channels = channels
  };
  auto packed = payload.pack();

  // Channel Count
  EXPECT_EQ(packed[1], 0x00);
  EXPECT_EQ(packed[2], 0x00);
  EXPECT_EQ(packed[3], 0x03);

  // Sample Rate
  EXPECT_EQ(packed[4], 0x00);
  EXPECT_EQ(packed[5], 0x03);

  // channel_id, 0 (24 bits, 3 bytes)
  EXPECT_EQ(packed[6], 0x00);
  EXPECT_EQ(packed[7], 0x00);
  EXPECT_EQ(packed[8], 0x00);

  // ch 0 num_samples, 3 (16 bits, 2 bytes)
  EXPECT_EQ(packed[9], 0x00);
  EXPECT_EQ(packed[10], 0x03);

  // ch 0 channel_data, 1, 2, 3 (12 bits, 1.5 bytes each)
  EXPECT_EQ(packed[11], 0x00);
  EXPECT_EQ(packed[12], 0x10);
  EXPECT_EQ(packed[13], 0x02);
  EXPECT_EQ(packed[14], 0x00);
  EXPECT_GE(packed[15], 0x03);

  // channel_id, 1 (24 bits, 3 bytes)
  EXPECT_EQ(packed[15], 0x30);
  EXPECT_EQ(packed[16], 0x00);
  EXPECT_EQ(packed[17], 0x00);
  EXPECT_GE(packed[18], 0x10);

  // ch 1 num_samples, 3 (16 bits, 2 bytes)
  EXPECT_EQ(packed[18], 0x10);
  EXPECT_EQ(packed[19], 0x00);
  EXPECT_GE(packed[20], 0x30);

  auto unpacked = NDTPPayloadBroadband::unpack(packed);
  EXPECT_EQ(unpacked.bit_width, bit_width);
  EXPECT_EQ(unpacked.is_signed, is_signed);
  EXPECT_EQ(unpacked.sample_rate, sample_rate);
  EXPECT_EQ(unpacked.channels.size(), channels.size());

  EXPECT_EQ(unpacked.channels[0].channel_id, 0);

  EXPECT_EQ(unpacked.channels[1].channel_id, 1);
  EXPECT_EQ(unpacked.channels[1].channel_data, std::vector<uint64_t>({4, 5, 6}));

  EXPECT_EQ(unpacked.channels[2].channel_id, 2);
  EXPECT_EQ(unpacked.channels[2].channel_data, std::vector<uint64_t>({3000, 2000, 1000}));

  EXPECT_EQ(packed[0] >> 1, bit_width);
}

TEST(NDTPTest, NDTPPayloadSpiketrainPackUnpack) {
  std::vector<uint8_t> spike_counts = {1, 2, 3, 2, 1};
  NDTPPayloadSpiketrain payload{
    .spike_counts = spike_counts,
    .bin_size_ms = 1,
  };
  auto packed = payload.pack();
  auto unpacked = NDTPPayloadSpiketrain::unpack(packed);
  EXPECT_EQ(unpacked.spike_counts, spike_counts);
  EXPECT_EQ(unpacked.bin_size_ms, 1);
}

TEST(NDTPTest, NDTPMessagePackUnpack) {
  uint8_t bit_width = 12;
  uint16_t sample_rate = 3;
  bool is_signed = false;
  NDTPHeader header {
    .data_type = synapse::DataType::kBroadband,
    .timestamp = 1234567890,
    .seq_number = 42
  };
  NDTPPayloadBroadband payload {
    .is_signed = is_signed,
    .bit_width = bit_width,
    .sample_rate = sample_rate,
    .channels = {
      NDTPPayloadBroadband::ChannelData {
        .channel_id = 0,
        .channel_data = {0}
      },
      NDTPPayloadBroadband::ChannelData {
        .channel_id = 1,
        .channel_data = {0, 1}
      },
      NDTPPayloadBroadband::ChannelData {
        .channel_id = 2,
        .channel_data = {0, 1, 2}
      }
    }
  };
  NDTPMessage message {
    .header = header,
    .payload = payload
  };

  auto b = std::get<NDTPPayloadBroadband>(message.payload);

  auto packed = message.pack();

  EXPECT_EQ(packed[0], 0x01);
  EXPECT_EQ(packed[1], static_cast<uint8_t>(synapse::DataType::kBroadband));
  EXPECT_EQ(packed[2], 0);
  EXPECT_EQ(packed[3], 0);
  EXPECT_EQ(packed[4], 0);
  EXPECT_EQ(packed[5], 0);
  EXPECT_EQ(packed[6], 0x49);
  EXPECT_EQ(packed[7], 0x96);
  EXPECT_EQ(packed[8], 0x02);
  EXPECT_EQ(packed[9], 0xD2);

  uint16_t crc = (packed[packed.size() - 2] << 8) | packed[packed.size() - 1];
  EXPECT_EQ(crc, 24287);

  auto unpacked = NDTPMessage::unpack(packed);
  EXPECT_EQ(unpacked.header, header) << "header is not equal to unpacked header";
  EXPECT_EQ(std::get<NDTPPayloadBroadband>(unpacked.payload), payload) << "payload is not equal to unpacked payload";

  NDTPHeader header2 {
    .data_type = synapse::DataType::kSpiketrain,
    .timestamp = 1234567890,
    .seq_number = 42
  };
  NDTPPayloadSpiketrain payload2 {
    .bin_size_ms = 1,
    .spike_counts = std::vector<uint8_t>{1, 2, 3, 2, 1}
  };
  NDTPMessage message2 {
    .header = header2,
    .payload = payload2
  };
  auto packed2 = message2.pack();

  EXPECT_EQ(packed2[0], 0x01);
  EXPECT_EQ(packed2[1], static_cast<uint8_t>(synapse::DataType::kSpiketrain));
  EXPECT_EQ(packed2[2], 0);
  EXPECT_EQ(packed2[3], 0);
  EXPECT_EQ(packed2[4], 0);
  EXPECT_EQ(packed2[5], 0);
  EXPECT_EQ(packed2[6], 0x49);
  EXPECT_EQ(packed2[7], 0x96);
  EXPECT_EQ(packed2[8], 0x02);
  EXPECT_EQ(packed2[9], 0xD2);

  // Sample count
  EXPECT_EQ(packed2[12], 0x00);
  EXPECT_EQ(packed2[13], 0x00);
  EXPECT_EQ(packed2[14], 0x00);
  EXPECT_EQ(packed2[15], 0x05);

  // Bin size
  EXPECT_EQ(packed2[16], 0x01);

  // Spike counts
  EXPECT_EQ(packed2[17], 0x6e);  // 1,2,3,2 -> 01101110 -> 0x6E
  EXPECT_EQ(packed2[18], 0x40);  // 1 -> 01000000 -> 0x40

  uint16_t crc2 = (packed2[packed2.size() - 2] << 8) | packed2[packed2.size() - 1];
  EXPECT_EQ(crc2, 46076);

  auto unpacked2 = NDTPMessage::unpack(packed2);

  EXPECT_EQ(unpacked2.header, header2) << "header2 is not equal to unpacked2.header";

  EXPECT_EQ(std::get<NDTPPayloadSpiketrain>(unpacked2.payload), payload2) << "payload2 is not equal to unpacked2.payload";
}

}  // namespace science::libndtp
