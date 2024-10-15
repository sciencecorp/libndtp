#pragma once

#include <tuple>
#include <utility>
#include <vector>

namespace science::libndtp {

using ByteArray = std::vector<uint8_t>;
using BitOffset = int;

/**
 * Packs a list of integers into a byte array with the specified bit width.
 * Handles both signed and unsigned integers.
 */
std::pair<ByteArray, BitOffset> to_bytes(
    const std::vector<int>& values, uint8_t bit_width, const ByteArray& existing = ByteArray(), int writing_bit_offset = 0,
    bool signed_val = false, bool le = false
);

/**
 * Parses a list of integers from a byte array with the specified bit width.
 * Returns the extracted integers, the new bit offset, and the remaining data.
 */
std::tuple<std::vector<int>, BitOffset, ByteArray> to_ints(
    const ByteArray& data, uint8_t bit_width, int count = 0, int start_bit = 0, bool signed_val = false,
    bool le = false
);

}  // namespace science::libndtp
