#include <gtest/gtest.h>
#include <science/libndtp/ndtp.hpp>
#include <science/libndtp/types.hpp>

namespace libndtp {

TEST(NDTPTest, NDTPHeaderPackUnpack) {
  NDTPHeader header{.data_type = synapse::DataType::kBroadband, .timestamp = 1234567890, .seq_number = 42};
  std::cout << "header: " << header.version << std::endl;
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
  insufficient_data.push_back(constants::kNDTPVersion);
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

  NDTPPayloadBroadband payload{.is_signed = is_signed, .bit_width = bit_width, .sample_rate = sample_rate, .channels = channels};
  auto packed = payload.pack();
  auto unpacked = NDTPPayloadBroadband::unpack(packed);
  EXPECT_EQ(unpacked.bit_width, bit_width);
  EXPECT_EQ(unpacked.is_signed, is_signed);
  EXPECT_EQ(unpacked.sample_rate, sample_rate);
  EXPECT_EQ(unpacked.channels.size(), channels.size());

  EXPECT_EQ(unpacked.channels[0].channel_id, 0);

  EXPECT_EQ(unpacked.channels[1].channel_id, 1);
  EXPECT_EQ(unpacked.channels[1].channel_data, std::vector<int>({4, 5, 6}));

  EXPECT_EQ(unpacked.channels[2].channel_id, 2);
  EXPECT_EQ(unpacked.channels[2].channel_data, std::vector<int>({3000, 2000, 1000}));

  EXPECT_EQ(packed[0] >> 1, bit_width);

  EXPECT_EQ((packed[1] << 16) | (packed[2] << 8) | packed[3], sample_rate);
}

// TEST(NDTPTest, NDTP_PAYLOAD_SPIKETRAIN_DATA) {
//   std::vector<int> spike_counts = {1, 2, 3, 2, 1};
//   NDTPPayloadSpiketrain payload(spike_counts);
//   auto packed = payload.pack();
//   auto unpacked = NDTPPayloadSpiketrain::unpack(packed);
//   EXPECT_EQ(unpacked.spike_counts, spike_counts);
// }

// TEST(NDTPTest, NDTP_MESSAGE) {
//   uint32_t bit_width = 12;
//   uint32_t sample_rate = 3;
//   bool is_signed = false;
//   NDTPHeader header(synapse::DataType::kBroadband, 1234567890, 42);
//   NDTPPayloadBroadband payload(is_signed, bit_width, sample_rate, {NDTPPayloadBroadbandChannelData(0, {1, 2, 3})});
//   NDTPMessage message(header, payload);
//   auto packed = message.pack();
//   auto unpacked = NDTPMessage::unpack(packed);
//   EXPECT_EQ(unpacked.header, header);
//   EXPECT_EQ(std::get<NDTPPayloadBroadband>(unpacked.payload), payload);

//   // NDTPPayloadSpiketrain payload2(std::vector<int>{1, 2, 3, 2, 1});
//   // NDTPMessage message2(header, payload2);
//   // auto packed2 = message2.pack();
//   // auto unpacked2 = NDTPMessage::unpack(packed2);
//   // EXPECT_EQ(unpacked2.header, header);
//   // EXPECT_EQ(std::get<NDTPPayloadSpiketrain>(unpacked2.payload), payload2);
// }

}  // namespace synapse
