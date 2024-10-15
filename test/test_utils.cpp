#include <gtest/gtest.h>
#include <science/libndtp/ndtp.h>
#include <science/libndtp/types.h>

namespace science::libndtp {

TEST(UtilsTest, ToBytesBasicFunctionality) {
  auto [result1, offset1] = to_bytes({1, 2, 3, 0}, 2);
  EXPECT_EQ(result1, (std::vector<uint8_t>{0x6C}));
  EXPECT_EQ(offset1, 0);

  auto [result2, offset2] = to_bytes({1, 2, 3, 2, 1}, 2);
  EXPECT_EQ(result2, (std::vector<uint8_t>{0x6E, 0x40}));
  EXPECT_EQ(offset2, 2);

  auto [result3, offset3] = to_bytes({7, 5, 3, 1}, 12);
  EXPECT_EQ(result3, (std::vector<uint8_t>{0x00, 0x70, 0x05, 0x00, 0x30, 0x01}));
  EXPECT_EQ(offset3, 0);

  auto [result4, offset4] = to_bytes({-7, -5, -3, -1}, 12, {}, 0, true);
  EXPECT_EQ(result4, (std::vector<uint8_t>{0xFF, 0x9F, 0xFB, 0xFF, 0xDF, 0xFF}));
  EXPECT_EQ(offset4, 0);

  auto [result5, offset5] = to_bytes({7, 5, 3}, 12, {0x01, 0x00}, 4);
  EXPECT_EQ(result5, (std::vector<uint8_t>{0x01, 0x00, 0x07, 0x00, 0x50, 0x03}));
  EXPECT_EQ(offset5, 0);

  auto [result6, offset6] = to_bytes({-7, -5, -3}, 12, {0x01, 0x00}, 4, true);
  EXPECT_EQ(result6, (std::vector<uint8_t>{0x01, 0x0F, 0xF9, 0xFF, 0xBF, 0xFD}));
  EXPECT_EQ(offset6, 0);

  auto [result7, offset7] = to_bytes({7, 5, 3}, 12);
  EXPECT_EQ(result7, (std::vector<uint8_t>{0x00, 0x70, 0x05, 0x00, 0x30}));
  EXPECT_EQ(offset7, 4);

  auto [result8, offset8] = to_bytes({1, 2, 3, 4}, 8);
  EXPECT_EQ(result8, (std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}));
  EXPECT_EQ(offset8, 0);

  std::vector<uint8_t> result9;
  int offset9;
  std::tie(result9, offset9) = to_bytes({7, 5, 3}, 12);
  EXPECT_EQ(result9, (std::vector<uint8_t>{0x00, 0x70, 0x05, 0x00, 0x30}));
  EXPECT_EQ(result9.size(), 5);
  EXPECT_EQ(offset9, 4);

  auto [result10, offset10] = to_bytes({3, 5, 7}, 12, result9, offset9);
  EXPECT_EQ(result10, (std::vector<uint8_t>{0x00, 0x70, 0x05, 0x00, 0x30, 0x03, 0x00, 0x50, 0x07}));
  EXPECT_EQ(result10.size(), 9);
  EXPECT_EQ(offset10, 0);
}

TEST(UtilsTest, ToBytesErrorCases) {
  // Test case: 8 doesn't fit in 3 bits
  EXPECT_THROW(to_bytes({8}, 3), std::invalid_argument);

  // Test case: Invalid bit width
  EXPECT_THROW(to_bytes({1, 2, 3, 0}, 0), std::invalid_argument);
}

TEST(UtilsTest, ToIntsBasicFunctionality) {
  std::vector<int> res;
  int offset;
  std::vector<uint8_t> remaining;

  std::tie(res, offset, remaining) = to_ints({0x6C}, 2);
  EXPECT_EQ(res, std::vector<int>({1, 2, 3, 0}));
  EXPECT_EQ(offset, 8);

  std::tie(res, offset, remaining) = to_ints({0x6C}, 2, 3);
  EXPECT_EQ(res, std::vector<int>({1, 2, 3}));
  EXPECT_EQ(offset, 6);

  std::tie(res, offset, remaining) = to_ints({0x00, 0x70, 0x05, 0x00, 0x30, 0x01}, 12);
  EXPECT_EQ(res, std::vector<int>({7, 5, 3, 1}));
  EXPECT_EQ(offset, 48);

  std::tie(res, offset, remaining) = to_ints({0x6C}, 2, 3, 2);
  EXPECT_EQ(res, std::vector<int>({2, 3, 0}));
  EXPECT_EQ(offset, 6 + 2);

  std::tie(res, offset, remaining) = to_ints({0x00, 0x07, 0x00, 0x50, 0x03}, 12, 3, 4);
  EXPECT_EQ(res, std::vector<int>({7, 5, 3}));
  EXPECT_EQ(offset, 36 + 4);

  std::tie(res, offset, remaining) = to_ints({0xFF, 0xF9, 0xFF, 0xBF, 0xFD}, 12, 3, 4, true);
  EXPECT_EQ(res, std::vector<int>({-7, -5, -3}));
  EXPECT_EQ(offset, 36 + 4);
}

TEST(UtilsTest, ToIntsByteArrayIteration) {
  std::vector<uint8_t> arry = {0x6E, 0x40};
  std::vector<int> res;
  int offset;

  std::tie(res, offset, arry) = to_ints(arry, 2, 1);
  EXPECT_EQ(res, std::vector<int>({1}));
  EXPECT_EQ(offset, 2);

  std::tie(res, offset, arry) = to_ints(arry, 2, 1, offset);
  EXPECT_EQ(res, std::vector<int>({2}));
  EXPECT_EQ(offset, 4);

  std::tie(res, offset, arry) = to_ints(arry, 2, 1, offset);
  EXPECT_EQ(res, std::vector<int>({3}));
  EXPECT_EQ(offset, 6);

  std::tie(res, offset, arry) = to_ints(arry, 2, 1, offset);
  EXPECT_EQ(res, std::vector<int>({2}));
  EXPECT_EQ(offset, 8);
}

TEST(UtilsTest, ToIntsErrorCases) {
  // Invalid bit width
  EXPECT_THROW(to_ints({0x01}, 0), std::invalid_argument);

  // Incomplete value
  EXPECT_THROW(to_ints({0x01}, 3), std::invalid_argument);

  // Insufficient data
  EXPECT_THROW(to_ints({0x01, 0x02}, 3), std::invalid_argument);
}

} // namespace science::libndtp
