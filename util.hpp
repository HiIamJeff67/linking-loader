#ifndef LINKING_LOADER_UTIL_HPP
#define LINKING_LOADER_UTIL_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace util {

std::vector<std::string> read_text_lines(const std::string& filename);
std::string strip_newline(const std::string& line);
std::string slice_or_empty(const std::string& line, std::size_t start, std::size_t len);

std::string trim_ascii_whitespace(const std::string& token);
std::string first_token(const std::string& raw_name);

std::uint32_t parse_hex_u32(const std::string& text, const char* field_name);

std::int64_t decode_twos_complement_hex(const std::string& hex_str, std::uint32_t bits);
std::string encode_twos_complement_hex(std::int64_t value, std::uint32_t nibbles);

}  // namespace util

#endif  // LINKING_LOADER_UTIL_HPP
