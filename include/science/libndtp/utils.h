#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace science::libndtp {

using ByteArray = std::vector<uint8_t>;
using BitOffset = size_t;

inline uint16_t crc16(const ByteArray& data, uint16_t poly = 0x8005, uint16_t init = 0xFFFF) {
  uint16_t crc = init;
  for (const auto& byte : data) {
    crc ^= byte << 8;
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ poly;
      } else {
        crc <<= 1;
      }
      crc &= 0xFFFF;
    }
  }
  return crc & 0xFFFF;
}

/**
 * Packs a list of integers into a byte array with the specified bit width.
 * Handles both signed and unsigned integers.
 */
template <typename T>
std::pair<ByteArray, BitOffset> to_bytes(
    const std::vector<T>& values,
    uint8_t bit_width,
    const ByteArray& existing = {},
    size_t writing_bit_offset = 0,
    bool is_signed = false,
    bool is_le = false
) {
  if (bit_width <= 0) {
    throw std::invalid_argument("to unpack bytes, bit width must be > 0 (value: " + std::to_string(bit_width) + ")");
  }

  size_t truncate_bytes = writing_bit_offset / 8;
  writing_bit_offset = writing_bit_offset % 8;

  ByteArray result = existing;
  if (!existing.empty()) {
    if (truncate_bytes < static_cast<int>(existing.size())) {
      result.erase(result.begin(), result.begin() + truncate_bytes);
    } else {
      result.clear();
    }
  }

  bool continue_last = !existing.empty() && writing_bit_offset > 0;
  uint8_t current_byte = (continue_last && !result.empty()) ? result.back() : 0;
  int bits_in_current_byte = writing_bit_offset;

  for (const auto& value : values) {
    int val = value;
    if (is_signed) {
      if (val < 0) {
        val = (1 << bit_width) + val;
      }
    } 

    int remaining_bits = bit_width;
    while (remaining_bits > 0) {
      int available_bits = 8 - bits_in_current_byte;
      int bits_to_write = std::min(available_bits, remaining_bits);

      int shift = remaining_bits - bits_to_write;
      int bits_add = (val >> shift) & ((1 << bits_to_write) - 1);

      if (is_le) {
        current_byte |= bits_add << bits_in_current_byte;
      } else {
        current_byte |= bits_add << (available_bits - bits_to_write);
      }

      remaining_bits -= bits_to_write;
      bits_in_current_byte += bits_to_write;

      if (bits_in_current_byte == 8) {
        if (continue_last) {
          result.back() = current_byte;
          continue_last = false;
        } else {
          result.push_back(current_byte);
        }
        current_byte = 0;
        bits_in_current_byte = 0;
      }
    }
  }

  if (bits_in_current_byte > 0) {
    result.push_back(current_byte);
  }

  return { result, bits_in_current_byte };
}


/**
 * Parses a list of integers from a byte array with the specified bit width.
 * Returns the extracted integers, the new bit offset, and the remaining data.
 */
template <typename T>
std::tuple<std::vector<T>, BitOffset, ByteArray> to_ints(
  const ByteArray& data,
  uint8_t bit_width,
  size_t count = 0,
  size_t start_bit = 0,
  bool is_signed = false,
  bool is_le = false
) {
  if (bit_width <= 0) {
    throw std::invalid_argument("to unpack ints, bit width must be > 0 (value: " + std::to_string(bit_width) + ")");
  }

  size_t truncate_bytes = start_bit / 8;
  size_t new_start_bit = start_bit % 8;

  ByteArray truncated_data = data;
  if (truncate_bytes < static_cast<int>(truncated_data.size())) {
    truncated_data.erase(truncated_data.begin(), truncated_data.begin() + truncate_bytes);
  } else {
    truncated_data.clear();
  }

  std::vector<T> values;
  T current_value = 0;
  size_t bits_in_current_value = 0;
  size_t total_bits_read = 0;
  BitOffset end_bit = new_start_bit;

  for (size_t byte_index = 0; byte_index < truncated_data.size(); ++byte_index) {
    uint8_t byte = truncated_data[byte_index];
    int start = (byte_index == 0) ? new_start_bit : 0;

    for (int bit_index = 7 - start; bit_index >= 0; --bit_index) {
      unsigned int bit = (byte >> bit_index) & 1;
      current_value = (current_value << 1) | bit;
      bits_in_current_value += 1;
      total_bits_read += 1;

      if (bits_in_current_value == bit_width) {
        if (is_signed && (current_value & (1 << (bit_width - 1)))) {
          current_value -= (1 << bit_width);
        } else {
          current_value &= ((1 << bit_width) - 1);
        }
        values.push_back(current_value);
        current_value = 0;
        bits_in_current_value = 0;

        if (count > 0 && static_cast<size_t>(values.size()) == count) {
          end_bit += total_bits_read;
          return { values, end_bit, truncated_data };
        }
      }
    }
  }

  if (bits_in_current_value > 0) {
    if (bits_in_current_value == bit_width) {
      if (is_signed && (current_value & (1 << (bit_width - 1)))) {
        current_value -= (1 << bit_width);
      } else {
        current_value &= ((1 << bit_width) - 1);
      }
      values.push_back(current_value);
    } else if (count == 0) {
      throw std::invalid_argument("Insufficient bits to form a complete value");
    }
  }

  if (count > 0 && values.size() > count) {
    values.resize(count);
  }

  end_bit += total_bits_read;
  return { values, end_bit, truncated_data};
}

}  // namespace science::libndtp
