// include/libndtp/Utilities.hpp
#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <tuple>

namespace libndtp {

using ByteArray = std::vector<uint8_t>;
using BitOffset = int;

/**
 * Packs a list of integers into a byte array with the specified bit width.
 * Handles both signed and unsigned integers.
 */
std::pair<ByteArray, BitOffset> to_bytes(
    const std::vector<int>& values,
    int bit_width,
    const ByteArray& existing = ByteArray(),
    int writing_bit_offset = 0,
    bool signed_val = false
);

/**
 * Parses a list of integers from a byte array with the specified bit width.
 * Returns the extracted integers, the new bit offset, and the remaining data.
 */
std::tuple<std::vector<int>, BitOffset, ByteArray> to_ints(
    const ByteArray& data,
    int bit_width,
    int count = 0,
    int start_bit = 0,
    bool signed_val = false
);

} // namespace libndtp