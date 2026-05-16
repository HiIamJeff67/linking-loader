#include "util.hpp"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace util {

std::vector<std::string> read_text_lines(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(strip_newline(line));
    }
    return lines;
}

std::string strip_newline(const std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        return line.substr(0, line.size() - 1);
    }
    return line;
}

std::string slice_or_empty(const std::string& line, std::size_t start, std::size_t len) {
    if (start >= line.size()) {
        return "";
    }
    return line.substr(start, len);
}

std::string trim_ascii_whitespace(const std::string& token) {
    std::size_t begin = 0;
    while (begin < token.size() && std::isspace(static_cast<unsigned char>(token[begin])) != 0) {
        ++begin;
    }

    std::size_t end = token.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(token[end - 1])) != 0) {
        --end;
    }

    return token.substr(begin, end - begin);
}

std::string first_token(const std::string& raw_name) {
    std::istringstream iss(raw_name);
    std::string name;
    iss >> name;
    return name;
}

std::uint32_t parse_hex_u32(const std::string& text, const char* field_name) {
    if (text.empty()) {
        throw std::runtime_error(std::string("Empty hexadecimal field: ") + field_name);
    }

    for (char c : text) {
        if (std::isxdigit(static_cast<unsigned char>(c)) == 0) {
            std::ostringstream oss;
            oss << "Non-hex character in " << field_name << ": '" << text << "'";
            throw std::runtime_error(oss.str());
        }
    }

    unsigned long value = 0;
    try {
        value = std::stoul(text, nullptr, 16);
    } catch (const std::exception&) {
        std::ostringstream oss;
        oss << "Failed to parse hexadecimal field " << field_name << ": '" << text << "'";
        throw std::runtime_error(oss.str());
    }

    if (value > std::numeric_limits<std::uint32_t>::max()) {
        std::ostringstream oss;
        oss << "Field exceeds 32-bit range " << field_name << ": '" << text << "'";
        throw std::runtime_error(oss.str());
    }

    return static_cast<std::uint32_t>(value);
}

std::int64_t decode_twos_complement_hex(const std::string& hex_str, std::uint32_t bits) {
    if (bits == 0 || bits > 63) {
        throw std::runtime_error("Unsupported bit width in decode_twos_complement_hex");
    }

    std::uint64_t raw = 0;
    try {
        raw = static_cast<std::uint64_t>(std::stoull(hex_str, nullptr, 16));
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to parse relocation field: " + hex_str);
    }

    const std::uint64_t sign_bit = 1ULL << (bits - 1U);
    if ((raw & sign_bit) != 0U) {
        return static_cast<std::int64_t>(raw - (1ULL << bits));
    }
    return static_cast<std::int64_t>(raw);
}

std::string encode_twos_complement_hex(std::int64_t value, std::uint32_t nibbles) {
    const std::uint32_t bits = nibbles * 4U;
    if (bits == 0 || bits > 63) {
        throw std::runtime_error("Unsupported bit width in encode_twos_complement_hex");
    }

    const std::uint64_t mod = (1ULL << bits);
    const std::uint64_t wrapped = static_cast<std::uint64_t>(
        (value % static_cast<std::int64_t>(mod) + static_cast<std::int64_t>(mod)) % static_cast<std::int64_t>(mod));

    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(static_cast<int>(nibbles)) << std::setfill('0') << wrapped;
    return oss.str();
}

}  // namespace util
