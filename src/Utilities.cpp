// src/Utilities.cpp
#include "libndtp/Utilities.hpp"
#include <cmath>

namespace libndtp {

std::pair<ByteArray, BitOffset> to_bytes(
    const std::vector<int>& values,
    int bit_width,
    const ByteArray& existing,
    int writing_bit_offset,
    bool signed_val
) {
    if (bit_width <= 0) {
        throw std::invalid_argument("bit width must be > 0");
    }

    int truncate_bytes = writing_bit_offset / 8;
    writing_bit_offset = writing_bit_offset % 8;

    ByteArray result = existing;
    if (!existing.empty()) {
        // Truncate the existing array
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
        if (signed_val) {
            int min_value = -(1 << (bit_width - 1));
            int max_value = (1 << (bit_width - 1)) - 1;
            if (val < min_value || val > max_value) {
                throw std::invalid_argument("signed value " + std::to_string(val) + " doesn't fit in " + std::to_string(bit_width) + " bits");
            }
            if (val < 0) {
                val = (1 << bit_width) + val;
            }
        }
        else {
            if (val < 0) {
                throw std::invalid_argument("unsigned packing specified, but value is negative");
            }
            if (val >= (1 << bit_width)) {
                throw std::invalid_argument("unsigned value " + std::to_string(val) + " doesn't fit in " + std::to_string(bit_width) + " bits");
            }
        }

        int remaining_bits = bit_width;
        while (remaining_bits > 0) {
            int available_bits = 8 - bits_in_current_byte;
            int bits_to_write = std::min(available_bits, remaining_bits);

            int shift = remaining_bits - bits_to_write;
            int bits_add = (val >> shift) & ((1 << bits_to_write) - 1);

            current_byte |= bits_add << (available_bits - bits_to_write);

            remaining_bits -= bits_to_write;
            bits_in_current_byte += bits_to_write;

            if (bits_in_current_byte == 8) {
                if (continue_last) {
                    result.back() = current_byte;
                    continue_last = false;
                }
                else {
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

    return {result, bits_in_current_byte};
}

std::tuple<std::vector<int>, BitOffset, ByteArray> to_ints(
    const ByteArray& data,
    int bit_width,
    int count,
    int start_bit,
    bool signed_val
) {
    if (bit_width <= 0) {
        throw std::invalid_argument("bit width must be > 0");
    }

    int truncate_bytes = start_bit / 8;
    int new_start_bit = start_bit % 8;

    ByteArray truncated_data = data;
    if (truncate_bytes < static_cast<int>(truncated_data.size())) {
        truncated_data.erase(truncated_data.begin(), truncated_data.begin() + truncate_bytes);
    }
    else {
        truncated_data.clear();
    }

    std::vector<int> values;
    int current_value = 0;
    int bits_in_current_value = 0;
    int total_bits_read = 0;
    BitOffset end_bit = new_start_bit;

    for (size_t byte_index = 0; byte_index < truncated_data.size(); ++byte_index) {
        uint8_t byte = truncated_data[byte_index];
        int start = (byte_index == 0) ? new_start_bit : 0;

        for (int bit_index = 7 - start; bit_index >= 0; --bit_index) {
            int bit = (byte >> bit_index) & 1;
            current_value = (current_value << 1) | bit;
            bits_in_current_value += 1;
            total_bits_read += 1;

            if (bits_in_current_value == bit_width) {
                if (signed_val && (current_value & (1 << (bit_width - 1)))) {
                    current_value -= (1 << bit_width);
                }
                else {
                    current_value &= ((1 << bit_width) - 1);
                }
                values.push_back(current_value);
                current_value = 0;
                bits_in_current_value = 0;

                if (count > 0 && static_cast<int>(values.size()) == count) {
                    end_bit += total_bits_read;
                    return {values, end_bit, truncated_data};
                }
            }
        }
    }

    if (bits_in_current_value > 0) {
        if (bits_in_current_value == bit_width) {
            if (signed_val && (current_value & (1 << (bit_width - 1)))) {
                current_value -= (1 << bit_width);
            }
            else {
                current_value &= ((1 << bit_width) - 1);
            }
            values.push_back(current_value);
        }
        else if (count == 0) {
            throw std::invalid_argument("Insufficient bits to form a complete value");
        }
    }

    if (count > 0) {
        // Truncate to the requested count
        if (static_cast<int>(values.size()) > count) {
            values.resize(count);
        }
    }

    end_bit += total_bits_read;
    return {values, end_bit, truncated_data};
}

} // namespace libndtp